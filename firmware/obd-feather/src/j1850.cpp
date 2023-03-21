/*
 * Copyright (c) 2018 Alexander Wachter
 * Copyright (c) 2023 Gavin Hurlbut
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <kernel.h>
#include <sys/printk.h>
#include <device.h>
#include <drivers/gpio.h>

#include "modes.h"
#include "j1850.h"
#include "gpio_map.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(j1850, 3);

J1850Port j1850;

typedef struct {
	uint8_t length;
	uint8_t data[J1850_BUFFER_SIZE];
} j1850_buf_t;

typedef struct {
	uint32_t timestamp;		// in system clock ticks (1uS per tick)
	bool value;				// bit value after the transition	
} j1850_bit_t;


K_THREAD_STACK_DEFINE(j1850_rx_thread_stack, J1850_RX_THREAD_STACK_SIZE);
K_THREAD_STACK_DEFINE(j1850_tx_thread_stack, J1850_TX_THREAD_STACK_SIZE);

K_MSGQ_DEFINE(j1850_rx_msgq, sizeof(j1850_buf_t), 32, 4);
K_MSGQ_DEFINE(j1850_tx_msgq, sizeof(j1850_buf_t), 32, 4);
K_MSGQ_DEFINE(j1850_rx_bit_msgq, sizeof(j1850_bit_t), 32, 4);

void J1850Port::begin(void)
{
	k_sem_init(&_tx_done_sem, 0, 1);

	_rx_tid = k_thread_create(&_rx_thread_data, j1850_rx_thread_stack,
				    K_THREAD_STACK_SIZEOF(j1850_rx_thread_stack),
				    j1850_rx_thread, NULL, NULL, NULL,
				    J1850_RX_THREAD_PRIORITY, 0, K_NO_WAIT);
	if (!_rx_tid) {
		printk("ERROR spawning rx thread\n");
	}

	_tx_tid = k_thread_create(&_tx_thread_data, j1850_tx_thread_stack,
				    K_THREAD_STACK_SIZEOF(j1850_tx_thread_stack),
				    j1850_tx_thread, NULL, NULL, NULL,
				    J1850_TX_THREAD_PRIORITY, 0, K_NO_WAIT);
	if (!_tx_tid) {
		printk("ERROR spawning tx thread\n");
	}

	printk("Finished init.\n");
}

void J1850Port::setMode(operation_mode_t mode)
{
	if (mode == _mode) {
		return;
	}

	_mode = mode;
	_rx_bit_count = 0;
	_rx_shift_reg = 0x00;
	_rx_buffer_index = 0;

	if (MODE_IS_J1850(_mode)) {
		gpio_irq_enable(0);
		_initialized = true;
	} else {
		gpio_irq_disable(0);
		_initialized = false;
	}
}

void J1850Port::rx_thread(void)
{
	j1850_buf_t buffer;
	obd_packet_t packet;
	int status;
	int length;
	uint8_t sum;

	while (1) {
		status = k_msgq_get(&j1850_rx_msgq, &buffer, K_MSEC(100));

		if (status == 0 && MODE_IS_KLINE(_mode) && _initialized) {
			sum = checksum(buffer.data, buffer.length);
			if (sum != buffer.data[buffer.length]) {
				// Bad checksum.  Chuck it.
				continue;
			}

			if (buffer.data[2] == 0xF1) {
				// This is a message we just sent.  Chuck it.  Gotta love half-duplex.
				continue;
			}

			length = buffer.length - 3;
			packet.mode = _mode;
			packet.count = length;
			packet.service = buffer.data[3];
			packet.pid = buffer.data[4];
			packet.a = length >= 3 ? buffer.data[5] : 0x00;
			packet.b = length >= 4 ? buffer.data[6] : 0x00;
			packet.c = length >= 5 ? buffer.data[7] : 0x00;
			packet.d = length >= 6 ? buffer.data[8] : 0x00;
			packet.unused = length >= 7 ? buffer.data[9] : 0x00;

			obd2.receive(&packet);
		}
	}
}

void J1850Port::rx_bit_thread(void)
{
	j1850_bit_t bit;
	j1850_buf_t buffer;
	uint32_t duration;
	int status;

	while (1) {
		status = k_msgq_get(&j1850_rx_bit_msgq, &bit, K_MSEC(1));

		if (!MODE_IS_KLINE(_mode) || !_initialized || _transmitting) {
			continue;
		}

		if (status == 0) {
			_receiving = true;    
			if (_timing_index == INDEX_SOF_1) {
				_timing_index = INDEX_ACT_0;
			}

			// Duration in ticks (1uS/tick)
			duration = (uint32_t)((((uint64_t)bit.timestamp + (1LL << 32)) - _last_edge) & 0xFFFFFFFF);

			if (MODE_IS_PWM(_mode)) {
				processPWM(duration, bit.value);
			} else {
				processVPW(duration, bit.value);
			}

			_last_level = bit.value;
			_last_edge = bit.timestamp;

			if (_timing_index == INDEX_EOD || _timing_index == INDEX_EOF) {
				_timing_index = INDEX_SOF_1;
				continue;
			}

			if (_timing_index == INDEX_ABORT) {
				_timing_index = INDEX_BRK_1;
				_receiving = false;
				kickTX();
				continue;                  
			}
		}
		
		uint32_t time_idle = (uint32_t)((((uint64_t)k_cycle_get_32() + (1LL << 32)) - _last_edge) & 0xFFFFFFFF);
		int16_t idle = time_idle > 1000 ? 1000 : time_idle;
		int16_t ifs = MODE_IS_PWM(_mode) ? _pwm_timing[INDEX_IFS].nom : _vpw_timing[INDEX_IFS].nom;     

		if (idle >= ifs) {
			memcpy(buffer.data, _rx_buffer, _rx_buffer_index);
			buffer.length = _rx_buffer_index;
			k_msgq_put(&j1850_rx_msgq, &buffer, K_NO_WAIT);

			// We can now transmit.  Woohoo!
			_receiving = false;
			kickTX();
		}
	}
}

bool J1850Port::isInTiming(const j1850_timing_t *timings, uint8_t index, uint32_t duration)
{
  index = index > INDEX_COUNT - 1 ? INDEX_COUNT -1 : index; 
  int16_t value = duration > 32767 ? 32767 : duration;
  
  const j1850_timing_t *timing = timings + index;
  if (timing->nom < 0) {
    return false;
  }

  return (timing->rx_min <= value && (timing->rx_max >= value || timing->rx_max < 0));   
}

void J1850Port::processPWM(uint32_t duration, bool level)
{
	if (level == _last_level) {
		// we got out of sync here.  Hmmm.  Whatever
		return;
	}

	if (level) {
		// we saw a falling edge, positive pulse of width duration
		if (isInTiming(_pwm_timing, INDEX_BRK_1, duration)) {
			// We are receiving a BREAK
			_timing_index = INDEX_BRK_0;
		} else if (isInTiming(_pwm_timing, INDEX_SOF_1, duration)) {
			// We are receiving a SOF
			if (_timing_index != INDEX_SOF_1) {
				// What's going on?  we are not expecting this, abort
				_timing_index = INDEX_ABORT;     
			} else {
				_timing_index = INDEX_SOF_0;
			}
		} else if (isInTiming(_pwm_timing, INDEX_ACT_0, duration)) {
			_timing_index = INDEX_INACT_0;
		} else if (isInTiming(_pwm_timing, INDEX_ACT_1, duration)) {
			_timing_index = INDEX_INACT_1;
		} else {
			// This is messed up.  Let's abort
			_timing_index = INDEX_ABORT;    
		}
	} else {
		// we saw a rising edge, negative pulse of width duration
		if (_timing_index == INDEX_BRK_0 && isInTiming(_pwm_timing, _timing_index, duration)) {
			// That was a successful break incoming.   Now we wait for IFS
			_timing_index = INDEX_IFS;
		} else if (_timing_index == INDEX_IFS && isInTiming(_pwm_timing, _timing_index, duration)) {
			// We hit IFS.  We can now transmit, or get the next SOF
			_receiving = false;
			_timing_index = INDEX_SOF_1;
		} else if (_timing_index == INDEX_SOF_0 && isInTiming(_pwm_timing, _timing_index, duration)) {
			// That was the end of a SOF.  Now all the data be ours.timing
			_timing_index = INDEX_ACT_0;
		} else if (_timing_index == INDEX_INACT_0 && isInTiming(_pwm_timing, _timing_index, duration)) {
			// 0 bit
			_rx_shift_reg <<= 1;
			_rx_shift_reg |= 0;
			_rx_bit_count++;
		} else if (_timing_index == INDEX_INACT_1 && isInTiming(_pwm_timing, _timing_index, duration)) {
			// 1 bit
			_rx_shift_reg <<= 1;
			_rx_shift_reg |= 1;
			_rx_bit_count++;     
		} else if ((_timing_index == INDEX_INACT_0 || _timing_index == INDEX_INACT_1) && isInTiming(_pwm_timing, INDEX_EOF, duration)) {
			// end of frame
			if (_rx_bit_count) {
				// We have a partial byte in the shift register.  Abort and clear it.
				_rx_bit_count = 0;
				_timing_index = INDEX_ABORT;        
			} else {
				_timing_index = INDEX_EOF;
			}
		} else {
			// Huh?  Let's just abort
			_timing_index = INDEX_ABORT;
		}

		if (_rx_bit_count == 8) {
			if (_rx_buffer_index < J1850_BUFFER_SIZE) {
				_rx_buffer[_rx_buffer_index++] = _rx_shift_reg;
			}
			_rx_bit_count = 0;
		}
	}
}

void J1850Port::processVPW(uint32_t duration, bool level)
{
	if (level == _last_level) {
		// we got out of sync here.  Hmmm.  Whatever
		return;
	}

	bool found = false;
	bool bit = false;

	if (level) {
		// we saw a falling edge, positive pulse of width duration
		if (isInTiming(_pwm_timing, INDEX_BRK_1, duration)) {
			// We are receiving a BREAK
			_timing_index = INDEX_IFS;
		} else if (isInTiming(_pwm_timing, INDEX_SOF_1, duration)) {
			// We are receiving a SOF
			if (_timing_index != INDEX_SOF_1) {
				// What's going on?  we are not expecting this, abort
				_timing_index = INDEX_ABORT;     
			} else {
				_timing_index = INDEX_INACT_0;
			}
		} else if (isInTiming(_pwm_timing, INDEX_ACT_0, duration)) {
			if (_timing_index == INDEX_EOD) {
				// Normalization bit, dump it
				_timing_index = INDEX_INACT_0;
				return;        
			}
			found = true;
			bit = false;
			_timing_index = INDEX_INACT_0;
		} else if (isInTiming(_pwm_timing, INDEX_ACT_1, duration)) {
			if (_timing_index == INDEX_EOD) {
				// Normalization bit, dump it
				_timing_index = INDEX_INACT_1;
				return;        
			}
			found = true;
			bit = true;
			_timing_index = INDEX_INACT_1;
		} else {
			// This is messed up.  Let's abort
			_timing_index = INDEX_ABORT;    
		}
	} else {
		// we saw a rising edge, negative pulse of width duration
		if (_timing_index == INDEX_IFS && isInTiming(_pwm_timing, _timing_index, duration)) {
			// We hit IFS.  We can now transmit, or get the next SOF
			_receiving = false;
			_timing_index = INDEX_SOF_1;
		} else if (isInTiming(_pwm_timing, INDEX_INACT_0, duration)) {
			found = true;
			bit = false;
			_timing_index = INDEX_ACT_0;
		} else if (isInTiming(_pwm_timing, INDEX_INACT_1, duration)) {
			found = true;
			bit = true;
			_timing_index = INDEX_ACT_1;
		} else if (_timing_index == INDEX_INACT_0 || _timing_index == INDEX_INACT_1) {
			j1850_timing_index_t index = INDEX_COUNT;
			if (isInTiming(_pwm_timing, INDEX_EOF, duration)) {
				index = INDEX_EOF;
			} else if (isInTiming(_pwm_timing, INDEX_EOD, duration)) {
				index = INDEX_EOD;        
			}

			if (index != INDEX_COUNT) {
				// end of frame
				if (_rx_bit_count) {
					// We have a partial byte in the shift register.  Abort and clear it.
					_rx_bit_count = 0;
					_timing_index = INDEX_ABORT;        
				} else {
					_timing_index = index;
				}
			} else {
				_timing_index = INDEX_ABORT;
			}
		} else {
			// Huh?  Let's just abort
			_timing_index = INDEX_ABORT;
		}

		if (found) {
			_rx_shift_reg <<= 1;
			_rx_shift_reg |= bit;
			_rx_bit_count++;
		}    

		if (_rx_bit_count == 8) {
			if (_rx_buffer_index < J1850_BUFFER_SIZE) {
				_rx_buffer[_rx_buffer_index++] = _rx_shift_reg;
			}
			_rx_bit_count = 0;
		}
	}
}

void J1850Port::tx_thread(void)
{
	j1850_buf_t buffer;
	int status;

	while (1) {
		status = k_msgq_get(&j1850_tx_msgq, &buffer, K_MSEC(100));

		if (status == 0 && MODE_IS_KLINE(_mode) && _initialized) {
			// Start the actual send
			write(buffer.data, buffer.length);
			status = k_sem_take(&_tx_done_sem, K_FOREVER);
			k_sleep(K_MSEC(30));
		}
	}
}

bool J1850Port::send(obd_packet_t *packet)
{
	j1850_buf_t buffer;
	int length;

	if (!packet) {
		return false;
	}

	length = packet->count;

	if (length > 7) {
		return false;
	}

	buffer.length = length + 3;
	buffer.data[0] = 0xC0;
	buffer.data[1] = 0x6A;
	buffer.data[2] = 0xF1;
	buffer.data[3] = packet->service;
	buffer.data[4] = packet->pid;
	buffer.data[5] = length >= 3 ? packet->a : 0x00;
	buffer.data[6] = length >= 4 ? packet->b : 0x00;
	buffer.data[7] = length >= 5 ? packet->c : 0x00;
	buffer.data[8] = length >= 6 ? packet->d : 0x00;
	buffer.data[9] = length >= 7 ? packet->unused : 0x00;
	buffer.data[buffer.length] = checksum(buffer.data, buffer.length);

	int status = k_msgq_put(&j1850_tx_msgq, &buffer, K_FOREVER);
	return status == 0;
}

uint8_t J1850Port::checksum(uint8_t *buffer, uint8_t len)
{
	uint8_t sum = 0;
	for (uint8_t i = 0; i < len; i++) {
		sum += *(buffer++);
	}
	return sum;
}

void J1850Port::kickTX(void)
{
  	if (!isIdle()) {
    	return;
 	}

	k_sem_give(&_tx_done_sem);
}

int J1850Port::write(uint8_t *buffer, uint8_t len)
{
	if (len > J1850_BUFFER_SIZE) {
		return 0;
	}

	memcpy(_tx_buffer, buffer, len);
	_tx_buffer_len = len;
	_tx_buffer_index = 0;
	_transmitting = true;
	_tx_shift_reg = 0x00;
	_tx_bit_count = 0;

	const j1850_timing_t *timing = MODE_IS_PWM(_mode) ? &_pwm_timing[_timing_index] : &_vpw_timing[_timing_index];

	while (_transmitting) {
		switch (_timing_index) {
			case INDEX_SOF_1:
				_last_level = true;
				_timing_index = INDEX_SOF_0;
				break;        

			case INDEX_SOF_0:
				if (MODE_IS_VPW(_mode)) {
					_timing_index = get_next_tx_index();
					continue;          
				}

				_last_level = false;
				_timing_index = get_next_tx_index();
				break;

			case INDEX_ACT_0:
				_last_level = true;
				_timing_index = MODE_IS_PWM(_mode) ? INDEX_INACT_0 : get_next_tx_index();
				break;

			case INDEX_INACT_0:
				_last_level = false;
				_timing_index = get_next_tx_index();
				break;
			
			case INDEX_ACT_1:
				_last_level = true;
				_timing_index = MODE_IS_PWM(_mode) ? INDEX_INACT_1 : get_next_tx_index();
				break;

			case INDEX_INACT_1:
				_last_level = false;
				_timing_index = get_next_tx_index();
				break;

			case INDEX_BRK_0:
				_timing_index = INDEX_EOD;
				_last_level = false;
				break;

			case INDEX_BRK_1:
				_last_level = true;
				_timing_index = MODE_IS_VPW(_mode) ? INDEX_EOD : INDEX_BRK_0;
				break;
			
			case INDEX_EOD:
				_last_level = false;
				_transmitting = false;
				_rx_buffer_index = 0;
				_rx_bit_count = 0;
				_timing_index = INDEX_SOF_1;        
				break;

			default:
				break;
		}

		if (_last_level) {
			// set us up for measuring from last rising edge (PWM or VPW)      
			_last_edge = k_cycle_get_32();
		}

		gpio_output_set(0, _last_level);
		k_busy_wait((uint32_t)timing->nom);
	}

	return _tx_buffer_len;
}


j1850_timing_index_t J1850Port::get_next_tx_index(bool new_packet)
{
	if (_abort) {
		return _timing_index == INDEX_BRK_1 ? INDEX_BRK_0 : INDEX_BRK_1;
	}

	if (_tx_bit_count == 0) {
		if (_tx_buffer_index >= _tx_buffer_len) {
			return INDEX_EOD;
		}

		bool sof = _tx_buffer_index == 0;
		_tx_shift_reg = _tx_buffer[_tx_buffer_index++];
		_tx_bit_count = 8;
		
		if (sof) {
			return INDEX_SOF_1;
		}
	}

	bool next_bit = _tx_shift_reg & 0x80;
	_tx_bit_count--;
	_tx_shift_reg = (uint8_t)(((uint16_t)_tx_shift_reg << 1) & 0xFF);

	if (MODE_IS_VPW(_mode) && _last_level) {
		// Next bit is passive (VPW)
		return next_bit ? INDEX_INACT_1 : INDEX_INACT_0;
	} else {
		// Next bit is active (VPW) or it's PWM
		return next_bit ? INDEX_ACT_1 : INDEX_ACT_0;
	}
}

void J1850Port::rx_callback(void)
{
	j1850_bit_t bit = {
		.timestamp = k_cycle_get_32(),
		.value = gpio_input_get(0),
	};

	k_msgq_put(&j1850_rx_bit_msgq, &bit, K_NO_WAIT);
}

// Helpers

void j1850_init(void)
{
	j1850.begin();
}

void j1850_rx_thread(void *arg1, void *arg2, void *arg3) 
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	j1850.rx_thread();
}

void j1850_rx_bit_thread(void *arg1, void *arg2, void *arg3) 
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	j1850.rx_bit_thread();
}

void j1850_tx_thread(void *arg1, void *arg2, void *arg3) 
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	j1850.tx_thread();
}

void j1850_rx_callback(const struct device *port, struct gpio_callback *cb, gpio_port_pins_t pins)
{
	j1850.rx_callback();
}
