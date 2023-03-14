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

#define RX_THREAD_STACK_SIZE 512
#define RX_THREAD_PRIORITY 2
#define STATE_POLL_THREAD_STACK_SIZE 512
#define STATE_POLL_THREAD_PRIORITY 2
#define SLEEP_TIME K_MSEC(250)

#include "modes.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(can, 3);

#define STACKSIZE 512
#define PRIORITY 2

K_THREAD_STACK_DEFINE(rx_thread_stack, RX_THREAD_STACK_SIZE);
K_THREAD_STACK_DEFINE(poll_state_stack, STATE_POLL_THREAD_STACK_SIZE);

const struct device *can_dev;
const struct device *led_gpio_dev;

struct k_thread rx_thread_data;
struct k_thread poll_state_thread_data;
struct k_work state_change_work;
enum can_state current_state;
struct can_bus_err_cnt current_err_cnt;

CAN_DEFINE_MSGQ(can_rx_msgq, 256);
CAN_DEFINE_MSGQ(can_tx_msgq, 32);

void can_tx_irq_callback(uint32_t error_flags, void *arg)
{
	char *sender = (char *)arg;

	if (error_flags) {
		printk("Callback! error-code: %d\nSender: %s\n",
		       error_flags, sender);
	}
}

void can_rx_thread(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);
	const struct zcan_filter filter = {
		.id_type = CAN_EXTENDED_IDENTIFIER,
		.rtr = CAN_DATAFRAME,
		.id = 0,
		.rtr_mask = 1,
		.id_mask = 0
	};
	struct zcan_frame msg;
	int filter_id = -1;
	int status;
	operation_mode_t mode = MODE_IDLE;
	operation_mode_t new_mode;
	struct can_timing timing;

	while (1) {
		new_mode = get_operation_mode();
		if (new_mode != mode) {
			mode = new_mode;

			switch (mode) {
				case MODE_HS_CAN:
					status = can_calc_timing(can_dev, &timing, 1000000, 875);
					break;
				case MODE_MS_CAN:
					status = can_calc_timing(can_dev, &timing, 250000, 875);
					break;
				case MODE_SW_CAN:
					status = can_calc_timing(can_dev, &timing, 125000, 875);
					break;
				default:
					// Not CAN, disable
					status = -1;
					break;
			}
			
			if (status == 0) {
				can_set_timing(can_dev, &timing, NULL);

				if (filter_id == -1) {
					filter_id = can_attach_msgq(can_dev, &can_rx_msgq, &filter);
					printk("CAN filter id: %d\n", filter_id);
				}
			} else {
				if (filter_id != -1) {
					can_detach(can_dev, filter_id);
					filter_id = -1;
				}
			}
		}

		status = k_msgq_get(&can_rx_msgq, &msg, K_MSEC(100));

		if (status == 0 && MODE_IS_CAN(mode)) {
			/* msg.dlc - length */
			/* mst.data - buffer */
		}
	}
}

char *state_to_str(enum can_state state)
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

void can_poll_state_thread(void *unused1, void *unused2, void *unused3)
{
	struct can_bus_err_cnt err_cnt = {0, 0};
	struct can_bus_err_cnt err_cnt_prev = {0, 0};
	enum can_state state_prev = CAN_ERROR_ACTIVE;
	enum can_state state;
	operation_mode_t mode = MODE_IDLE;

	while (1) {
		mode = get_operation_mode();

		if (MODE_IS_CAN(mode)) {
			state = can_get_state(can_dev, &err_cnt);
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

void state_change_work_handler(struct k_work *work)
{
	operation_mode_t mode = get_operation_mode();

	printk("State Change ISR\nstate: %s\n"
	       "rx error count: %d\n"
	       "tx error count: %d\n",
		state_to_str(current_state),
		current_err_cnt.rx_err_cnt, current_err_cnt.tx_err_cnt);

	if (MODE_IS_CAN(mode) && current_state == CAN_BUS_OFF) {
		printk("Recover from bus-off\n");

		if (can_recover(can_dev, K_MSEC(100)) != 0) {
			printk("Recovery timed out\n");
		}
	}
}

void state_change_isr(enum can_state state, struct can_bus_err_cnt err_cnt)
{
	current_state = state;
	current_err_cnt = err_cnt;
	k_work_submit(&state_change_work);
}

void can_tx_thread(void)
{
	struct zcan_frame msg;
	k_tid_t can_rx_tid, can_get_state_tid;
	int status;
	operation_mode_t mode = MODE_IDLE;

	can_dev = device_get_binding(DT_CHOSEN_ZEPHYR_CAN_PRIMARY_LABEL);

	if (!can_dev) {
		printk("CAN: Device driver not found.\n");
		return;
	}

    k_work_init(&state_change_work, state_change_work_handler);

	can_rx_tid = k_thread_create(&rx_thread_data, rx_thread_stack,
				    K_THREAD_STACK_SIZEOF(rx_thread_stack),
				    can_rx_thread, NULL, NULL, NULL,
				    RX_THREAD_PRIORITY, 0, K_NO_WAIT);
	if (!can_rx_tid) {
		printk("ERROR spawning rx thread\n");
	}

	can_get_state_tid = k_thread_create(&poll_state_thread_data,
					poll_state_stack,
					K_THREAD_STACK_SIZEOF(poll_state_stack),
					can_poll_state_thread, NULL, NULL, NULL,
					STATE_POLL_THREAD_PRIORITY, 0,
					K_NO_WAIT);
	if (!can_get_state_tid) {
		printk("ERROR spawning poll_state_thread\n");
	}

	can_register_state_change_isr(can_dev, state_change_isr);

	printk("Finished init.\n");

	while (1) {
		mode = get_operation_mode();

		status = k_msgq_get(&can_tx_msgq, &msg, K_MSEC(100));

		if (status == 0 && MODE_IS_CAN(mode)) {
			/* This sending call is blocking until the message is sent. */
			can_send(can_dev, &msg, K_MSEC(100), NULL, NULL);
		}
	}
}

K_THREAD_DEFINE(can_tx_id, STACKSIZE, can_tx_thread,
                NULL, NULL, NULL, PRIORITY, 0, 0);

