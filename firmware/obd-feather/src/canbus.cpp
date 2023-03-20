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
#include <drivers/can.h>

#include "modes.h"
#include "canbus.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(canbus, 3);

CANBusPort canbus;

K_THREAD_STACK_DEFINE(canbus_rx_thread_stack, CAN_RX_THREAD_STACK_SIZE);
K_THREAD_STACK_DEFINE(canbus_tx_thread_stack, CAN_TX_THREAD_STACK_SIZE);
K_THREAD_STACK_DEFINE(canbus_poll_state_stack, CAN_STATE_POLL_THREAD_STACK_SIZE);

CAN_DEFINE_MSGQ(canbus_rx_msgq, 256);
CAN_DEFINE_MSGQ(canbus_tx_msgq, 32);

void CANBusPort::begin(void)
{
	_dev = device_get_binding(DT_CHOSEN_ZEPHYR_CAN_PRIMARY_LABEL);

	if (!_dev) {
		printk("CAN: Device driver not found.\n");
		return;
	}

    k_work_init(&_state_change_work, canbus_state_change_work_handler);

	_rx_tid = k_thread_create(&_rx_thread_data, canbus_rx_thread_stack,
				    K_THREAD_STACK_SIZEOF(canbus_rx_thread_stack),
				    canbus_rx_thread, NULL, NULL, NULL,
				    CAN_RX_THREAD_PRIORITY, 0, K_NO_WAIT);
	if (!_rx_tid) {
		printk("ERROR spawning rx thread\n");
	}

	_tx_tid = k_thread_create(&_tx_thread_data, canbus_tx_thread_stack,
				    K_THREAD_STACK_SIZEOF(canbus_tx_thread_stack),
				    canbus_tx_thread, NULL, NULL, NULL,
				    CAN_TX_THREAD_PRIORITY, 0, K_NO_WAIT);
	if (!_tx_tid) {
		printk("ERROR spawning tx thread\n");
	}

	_poll_state_tid = k_thread_create(&_poll_state_thread_data,
					canbus_poll_state_stack,
					K_THREAD_STACK_SIZEOF(canbus_poll_state_stack),
					canbus_poll_state_thread, NULL, NULL, NULL,
					CAN_STATE_POLL_THREAD_PRIORITY, 0, K_NO_WAIT);
	if (!_poll_state_tid) {
		printk("ERROR spawning poll_state_thread\n");
	}

	can_register_state_change_isr(_dev, canbus_state_change_isr);

	printk("Finished init.\n");
}

void CANBusPort::setMode(operation_mode_t mode)
{
	int status;
	struct can_timing timing;

	if (mode == _mode) {
		return;
	}

	_mode = mode;

	switch (_mode) {
		case MODE_HS_CAN:
			status = can_calc_timing(_dev, &timing, 500000, 875);
			break;
		case MODE_MS_CAN:
			status = can_calc_timing(_dev, &timing, 125000, 875);
			break;
		case MODE_SW_CAN:
			status = can_calc_timing(_dev, &timing, 83333, 875);
			break;
		default:
			// Not CAN, disable
			status = -1;
			break;
	}
	
	if (status == 0) {
		can_set_timing(_dev, &timing, NULL);

		const struct zcan_filter filter = {
			.id = 0,
			.rtr = CAN_DATAFRAME,
			.id_type = CAN_EXTENDED_IDENTIFIER,
			.id_mask = 0,
			.rtr_mask = 1,
		};

		if (_filter_id == -1) {
			_filter_id = can_attach_msgq(_dev, &canbus_rx_msgq, &filter);
			printk("CAN filter id: %d\n", _filter_id);
		}
	} else {
		if (_filter_id != -1) {
			can_detach(_dev, _filter_id);
			_filter_id = -1;
		}
	}
}

void CANBusPort::rx_thread(void)
{
	struct zcan_frame msg;
	int status;

	while (1) {
		status = k_msgq_get(&canbus_rx_msgq, &msg, K_MSEC(100));

		if (status == 0 && MODE_IS_CAN(_mode) && msg.dlc == 8) {
			obd_packet_t packet = {
				.mode = _mode,
				.id = msg.id,
				.count = msg.data[0],
				.service = msg.data[1],
				.pid = msg.data[2],
				.a = msg.data[3],
				.b = msg.data[4],
				.c = msg.data[5],
				.d = msg.data[6],
				.unused = msg.data[7],
			};

			obd2.receive(&packet);
		}
	}
}

void CANBusPort::tx_thread(void)
{
	int status;
	struct zcan_frame msg;

	while (1) {
		status = k_msgq_get(&canbus_tx_msgq, &msg, K_MSEC(100));

		if (status == 0 && MODE_IS_CAN(_mode)) {
			/* This sending call is blocking until the message is sent. */
			can_send(_dev, &msg, K_MSEC(100), NULL, NULL);
		}
	}
}

bool CANBusPort::send(obd_packet_t *packet)
{
	if (!packet) {
		return false;
	}

	if (packet->count > 7) {
		return false;
	}

	uint32_t id = packet->id;

	struct zcan_frame msg = {
		.id = id,
		.fd = 0,
		.rtr = CAN_DATAFRAME,
		.id_type = id < (1 << 11) ? CAN_STANDARD_IDENTIFIER : CAN_EXTENDED_IDENTIFIER,
		.dlc = 8,
		.data = {packet->count, packet->service, packet->pid, packet->a, packet->b, packet->c, packet->d, packet->unused},
	};

	int status = k_msgq_put(&canbus_tx_msgq, &msg, K_FOREVER);
	return status == 0;
}

const char *CANBusPort::state_to_str(enum can_state state)
{
	switch (state) {
	case CAN_ERROR_ACTIVE:
		return "error-active";
	case CAN_ERROR_PASSIVE:
		return "error-passive";
	case CAN_BUS_OFF:
		return "bus-off";
	default:
		return "unknown";
	}
}

void CANBusPort::poll_state_thread(void)
{
	struct can_bus_err_cnt err_cnt = {0, 0};
	struct can_bus_err_cnt err_cnt_prev = {0, 0};
	enum can_state state_prev = CAN_ERROR_ACTIVE;
	enum can_state state;

	while (1) {
		if (MODE_IS_CAN(_mode)) {
			state = can_get_state(_dev, &err_cnt);
			if (err_cnt.tx_err_cnt != err_cnt_prev.tx_err_cnt ||
				err_cnt.rx_err_cnt != err_cnt_prev.rx_err_cnt ||
				state_prev != state) {

				err_cnt_prev.tx_err_cnt = err_cnt.tx_err_cnt;
				err_cnt_prev.rx_err_cnt = err_cnt.rx_err_cnt;
				state_prev = state;
				printk("state: %s\n"
					   "rx error count: %d\n"
					   "tx error count: %d\n",
					   state_to_str(state),
					   err_cnt.rx_err_cnt, err_cnt.tx_err_cnt);
				continue;
			}
		} 
		
		k_sleep(K_MSEC(100));
	}
}

void CANBusPort::state_change_work_handler(struct k_work *work)
{
	printk("State Change ISR\nstate: %s\n"
	       "rx error count: %d\n"
	       "tx error count: %d\n",
		   state_to_str(_current_state),
		_current_err_cnt.rx_err_cnt, _current_err_cnt.tx_err_cnt);

	if (MODE_IS_CAN(_mode) && _current_state == CAN_BUS_OFF) {
		printk("Recover from bus-off\n");

		if (can_recover(_dev, K_MSEC(100)) != 0) {
			printk("Recovery timed out\n");
		}
	}
}

void CANBusPort::state_change_isr(enum can_state state, struct can_bus_err_cnt err_cnt)
{
	_current_state = state;
	_current_err_cnt = err_cnt;
	k_work_submit(&_state_change_work);
}


// Helpers

void canbus_init(void)
{
	canbus.begin();
}

void canbus_state_change_work_handler(struct k_work *work)
{
	canbus.state_change_work_handler(work);
}

void canbus_state_change_isr(enum can_state state, struct can_bus_err_cnt err_cnt)
{
	canbus.state_change_isr(state, err_cnt);
}

void canbus_rx_thread(void *arg1, void *arg2, void *arg3) 
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	canbus.rx_thread();
}

void canbus_tx_thread(void *arg1, void *arg2, void *arg3) 
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	canbus.tx_thread();
}

void canbus_poll_state_thread(void *arg1, void *arg2, void *arg3) 
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	canbus.poll_state_thread();
}
