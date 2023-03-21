/*
 * Copyright (c) 2023 Gavin Hurlbut
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <kernel.h>
#include <sys/printk.h>
#include <device.h>
#include <drivers/uart.h>

#include "modes.h"
#include "kline.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(kline, 3);

KLinePort kline;

typedef struct {
	uint8_t length;
	uint8_t data[KLINE_BUFFER_SIZE];
} kline_buf_t;

K_THREAD_STACK_DEFINE(kline_rx_thread_stack, KLINE_RX_THREAD_STACK_SIZE);
K_THREAD_STACK_DEFINE(kline_tx_thread_stack, KLINE_TX_THREAD_STACK_SIZE);

K_MSGQ_DEFINE(kline_rx_msgq, sizeof(kline_buf_t), 32, 4);
K_MSGQ_DEFINE(kline_tx_msgq, sizeof(kline_buf_t), 32, 4);

bool KLinePort::configure(uint32_t baud)
{
	const struct uart_config config = {
		.baudrate = baud,
		.parity = UART_PARITY_NONE,
		.stop_bits = UART_STOPBITS_1,
		.data_bits = UART_CFG_DATA_BITS_8,
		.flow_ctrl = UART_CFG_FLOW_CTRL_NONE,
	};

	return uart_configure(_dev, &config) == 0;
}

void KLinePort::begin(void)
{
	_initialized = false;
	_dev = DEVICE_DT_GET(DT_NODELABEL(usart6));

	if (!_dev) {
		printk("KLine: Device driver not found.\n");
		return;
	}

	uart_callback_set(_dev, kline_uart_callback, this);

	_mode = MODE_IDLE;
	disable();

	k_sem_init(&_tx_done_sem, 0, 1);
	k_sem_init(&_rx_rdy_sem, 0, 1);

	_rx_tid = k_thread_create(&_rx_thread_data, kline_rx_thread_stack,
				    K_THREAD_STACK_SIZEOF(kline_rx_thread_stack),
				    kline_rx_thread, NULL, NULL, NULL,
				    KLINE_RX_THREAD_PRIORITY, 0, K_NO_WAIT);
	if (!_rx_tid) {
		printk("ERROR spawning rx thread\n");
	}

	_tx_tid = k_thread_create(&_tx_thread_data, kline_tx_thread_stack,
				    K_THREAD_STACK_SIZEOF(kline_tx_thread_stack),
				    kline_tx_thread, NULL, NULL, NULL,
				    KLINE_TX_THREAD_PRIORITY, 0, K_NO_WAIT);
	if (!_tx_tid) {
		printk("ERROR spawning tx thread\n");
	}

	printk("Finished init.\n");
}

void KLinePort::setMode(operation_mode_t mode)
{
	if (mode == _mode) {
		return;
	}

	_mode = mode;

	if (MODE_IS_KLINE(_mode)) {
		init();

		if (!_initialized) {
			_mode = MODE_IDLE;
		}
	} 
	
	if (!MODE_IS_KLINE(_mode)) {
		disable();
	}
}

void KLinePort::init(void)
{
	_initialized = false;
	k_sleep(K_MSEC(3000));

	switch (_mode) {
		case MODE_ISO9141_5BAUD_INIT:
        case MODE_ISO14230_5BAUD_INIT:
			init_5baud();
			break;

        case MODE_ISO14230_FAST_INIT:
			init_fast();
			break;

		default:
			break;
	}
}

void KLinePort::init_5baud(void)
{
	int status;

	_initialized = false;

	if (!configure(5)) {
		return;
	}

	enable();

	uint8_t cmd = 0x33;
	uart_tx(_dev, &cmd, 1, SYS_FOREVER_MS);
	status = k_sem_take(&_tx_done_sem, K_MSEC(3000));
	if (status != 0) {
		return;
	}

	configure(10400);
	uint8_t response = 0x00;
	uart_rx_enable(_dev, &response, 1, 500);
	status = k_sem_take(&_rx_rdy_sem, K_MSEC(500));

	if (response != 0x55 || status != 0) {
		return;
	}

	uint8_t v1 = 0x00;
	uart_rx_enable(_dev, &v1, 1, 20);
	status = k_sem_take(&_rx_rdy_sem, K_MSEC(20));
	if (status != 0) {
		return;
	}

	uint8_t v2 = 0x00;
	uart_rx_enable(_dev, &v2, 1, 20);
	status = k_sem_take(&_rx_rdy_sem, K_MSEC(20));
	if (status != 0) {
		return;
	}

	if (_mode == MODE_ISO9141_5BAUD_INIT && v1 != v2) {
		return;
	}

	k_sleep(K_MSEC(30));

	v2 = ~v2;
	uart_tx(_dev, &v2, 1, SYS_FOREVER_MS);
	status = k_sem_take(&_tx_done_sem, K_MSEC(50));
	if (status != 0) {
		return;
	}

	uart_rx_enable(_dev, &response, 1, 50);
	status = k_sem_take(&_rx_rdy_sem, K_MSEC(50));
	if (response != 0xCC || status != 0) {
		return;
	}

	k_sleep(K_MSEC(50));
	_initialized = true;
}

void KLinePort::init_fast(void)
{
	int status;

	_initialized = false;

	// send 25ms low/25ms high - so 50ms at 10bit/symbol = 0x0F at 200baud.
	configure(200);
	uint8_t cmd = 0x0F;
	uart_tx(_dev, &cmd, 1, SYS_FOREVER_MS);
	status = k_sem_take(&_tx_done_sem, K_MSEC(60));
	if (status != 0) {
		return;
	}

	configure(10400);

	uint8_t msg[5] = {0xC1, 0x33, 0xF1, 0x81, 0x66};
	uart_tx(_dev, msg, 5, SYS_FOREVER_MS);
	status = k_sem_take(&_tx_done_sem, K_MSEC(10));
	if (status != 0) {
		return;
	}

	uint8_t response[6];
	uart_rx_enable(_dev, response, 6, 10);
	status = k_sem_take(&_rx_rdy_sem, K_MSEC(50));
	if (response[3] != 0xC1 || status != 0) {
		return;
	}

	_initialized = true;
}

void KLinePort::disable(void)
{
	uart_tx_abort(_dev);
	uart_rx_disable(_dev);

	uart_irq_err_disable(_dev);
	uart_irq_rx_disable(_dev);
	uart_irq_tx_disable(_dev);
	_initialized = false;
}

void KLinePort::enable(void)
{
	_rx_index = 0;
	uart_rx_enable(_dev, _rx_buffers[_rx_index], KLINE_BUFFER_SIZE, 15);

	uart_irq_rx_enable(_dev);
	uart_irq_tx_enable(_dev);
	uart_irq_err_enable(_dev);
}


uint8_t KLinePort::checksum(uint8_t *buffer, uint8_t len)
{
	uint8_t sum = 0;
	for (uint8_t i = 0; i < len; i++) {
		sum += *(buffer++);
	}
	return sum;
}

void KLinePort::rx_thread(void)
{
	kline_buf_t buffer;
	obd_packet_t packet;
	int status;
	int length;
	uint8_t sum;

	while (1) {
		status = k_msgq_get(&kline_rx_msgq, &buffer, K_MSEC(100));

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

void KLinePort::rx_ready_callback(uint8_t *buf, uint8_t offset, uint8_t len)
{
	if (!_initialized) {
		k_sem_give(&_rx_rdy_sem);
		return;
	}

	kline_buf_t buffer;
	buffer.length = len;
	memcpy(buffer.data, &buf[offset], len);
	k_msgq_put(&kline_rx_msgq, &buffer, K_NO_WAIT);

	if (offset + len >= KLINE_BUFFER_SIZE - 11) {
		// Want to be sure not to receive a partial frame due to hitting the end
		// of the buffer.  I wish zephyr would let me just invalidate the buffer
		// itself, but alas.
		uart_rx_disable(_dev);
		uart_rx_enable(_dev, _rx_buffers[_rx_index], KLINE_BUFFER_SIZE, 15);
	}
}

void KLinePort::rx_buf_request_callback(void)
{
	if (!_initialized) {
		return;
	}

	_rx_index++;
	_rx_index %= KLINE_BUFFER_COUNT;
	uart_rx_buf_rsp(_dev, _rx_buffers[_rx_index], KLINE_BUFFER_SIZE);
}

void KLinePort::rx_buf_released_callback(uint8_t *buf)
{
	// No need to do anything here, I think.
}

void KLinePort::rx_disabled_callback()
{
	// No need to do anything here, I think.
}

void KLinePort::rx_stopped_callback(uint8_t *buf, uint8_t offset, uint8_t len, enum uart_rx_stop_reason reason)
{
	ARG_UNUSED(buf);
	ARG_UNUSED(offset);
	ARG_UNUSED(reason);

	// Just start it back up if we are still supposed to be running
	if (_initialized && MODE_IS_KLINE(_mode)) {
		_initialized = false;
		init();
	}
}

void KLinePort::tx_thread(void)
{
	kline_buf_t buffer;
	int status;
	int sent;

	while (1) {
		status = k_msgq_get(&kline_tx_msgq, &buffer, K_MSEC(100));

		if (status == 0 && MODE_IS_KLINE(_mode) && _initialized) {
			status = uart_tx(_dev, buffer.data, buffer.length + 1, SYS_FOREVER_MS);
			k_sem_take(&_tx_done_sem, K_FOREVER);
			sent = _tx_sent;

			k_sleep(K_MSEC(30));
		}
	}
}

void KLinePort::tx_done_callback(int sent)
{
	_tx_sent = sent;
	k_sem_give(&_tx_done_sem);
}

bool KLinePort::send(obd_packet_t *packet)
{
	kline_buf_t buffer;
	int length;

	if (!packet) {
		return false;
	}

	length = packet->count;

	if (length > 7) {
		return false;
	}

	buffer.length = length + 3;
	buffer.data[0] = MODE_IS_ISO9141(_mode) ? 0x68 : (0xC0 | (length & 0x07));
	buffer.data[1] = MODE_IS_ISO9141(_mode) ? 0x6A : 0x33;
	buffer.data[2] = 0xF1;
	buffer.data[3] = packet->service;
	buffer.data[4] = packet->pid;
	buffer.data[5] = length >= 3 ? packet->a : 0x00;
	buffer.data[6] = length >= 4 ? packet->b : 0x00;
	buffer.data[7] = length >= 5 ? packet->c : 0x00;
	buffer.data[8] = length >= 6 ? packet->d : 0x00;
	buffer.data[9] = length >= 7 ? packet->unused : 0x00;
	buffer.data[buffer.length] = checksum(buffer.data, buffer.length);

	int status = k_msgq_put(&kline_tx_msgq, &buffer, K_FOREVER);
	return status == 0;
}


// Helpers

void kline_init(void)
{
	kline.begin();
}

void kline_rx_thread(void *arg1, void *arg2, void *arg3) 
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	kline.rx_thread();
}

void kline_tx_thread(void *arg1, void *arg2, void *arg3) 
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	kline.tx_thread();
}

void kline_uart_callback(const struct device *dev, struct uart_event *evt, void *user_data)
{
	if (!evt || !user_data) {
		return;
	}

	ARG_UNUSED(dev);

	KLinePort *port = static_cast<KLinePort *>(user_data);
	
	switch(evt->type) {
		case UART_TX_DONE:
		case UART_TX_ABORTED:
			port->tx_done_callback(evt->data.tx.len);
			break;

		case UART_RX_RDY:
			port->rx_ready_callback(evt->data.rx.buf, evt->data.rx.offset, evt->data.rx.len);
			break;

		case UART_RX_BUF_REQUEST:
			port->rx_buf_request_callback();
			break;

		case UART_RX_BUF_RELEASED:
			port->rx_buf_released_callback(evt->data.rx_buf.buf);
			break;

		case UART_RX_DISABLED:
			port->rx_disabled_callback();
			break;

		case UART_RX_STOPPED:
			port->rx_stopped_callback(evt->data.rx_stop.data.buf, evt->data.rx_stop.data.offset, evt->data.rx_stop.data.len, evt->data.rx_stop.reason);
			break;
	}
}
