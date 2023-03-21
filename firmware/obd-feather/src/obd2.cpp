#include <zephyr.h>

#include "obd2.h"
#include "gpio_map.h"
#include "modes.h"
#include "canbus.h"
#include "kline.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(obd2, 3);

OBD2 obd2;

K_THREAD_STACK_DEFINE(obd2_rx_thread_stack, OBD2_RX_THREAD_STACK_SIZE);
K_THREAD_STACK_DEFINE(obd2_tx_thread_stack, OBD2_TX_THREAD_STACK_SIZE);

K_MSGQ_DEFINE(obd2_rx_msgq, sizeof(obd_packet_t), 32, 4);
K_MSGQ_DEFINE(obd2_tx_msgq, sizeof(obd_packet_t), 32, 4);

void OBD2::begin(void)
{
    _port = 0;
    _mode = MODE_IDLE;

    _rx_tid = k_thread_create(&_rx_thread_data, obd2_rx_thread_stack,
				    K_THREAD_STACK_SIZEOF(obd2_rx_thread_stack),
				    obd2_rx_thread, NULL, NULL, NULL,
				    OBD2_RX_THREAD_PRIORITY, 0, K_NO_WAIT);
	if (!_rx_tid) {
		printk("ERROR spawning rx thread\n");
	}

	_tx_tid = k_thread_create(&_tx_thread_data, obd2_tx_thread_stack,
				    K_THREAD_STACK_SIZEOF(obd2_tx_thread_stack),
				    obd2_tx_thread, NULL, NULL, NULL,
				    OBD2_TX_THREAD_PRIORITY, 0, K_NO_WAIT);
	if (!_tx_tid) {
		printk("ERROR spawning tx thread\n");
	}
}

void OBD2::setMode(operation_mode_t mode)
{
    if (mode < 0 || mode >= MAX_MODE) {
        return;
    }

    k_mutex_lock(&_mutex, K_FOREVER);
    
    disable();
    enable(mode);

    k_mutex_unlock(&_mutex);
}

operation_mode_t OBD2::getMode(void)
{
    operation_mode_t mode;
    
    k_mutex_lock(&_mutex, K_FOREVER);
    mode = _mode;
    k_mutex_unlock(&_mutex);

    return mode;
}

bool OBD2::send(obd_packet_t *packet)
{
    if (_port) {
        return k_msgq_put(&obd2_tx_msgq, packet, K_FOREVER);
    }
    return false;
}

bool OBD2::receive(obd_packet_t *packet)
{
    return k_msgq_put(&obd2_rx_msgq, packet, K_FOREVER);
}

void OBD2::enable(operation_mode_t mode)
{
    switch (mode) {
        case MODE_HS_CAN:
            gpio_output_set(GPIO_CAN_SEL0, false);
            gpio_output_set(GPIO_CAN_SEL1, true);
            gpio_output_set(GPIO_CAN_EN, true);
            _port = &canbus;
            break;
        case MODE_MS_CAN:
            gpio_output_set(GPIO_CAN_SEL0, true);
            gpio_output_set(GPIO_CAN_SEL1, false);
            gpio_output_set(GPIO_CAN_EN, true);
            _port = &canbus;
            break;
        case MODE_SW_CAN:
            gpio_output_set(GPIO_CAN_SEL0, true);
            gpio_output_set(GPIO_CAN_SEL1, true);
            _port = &canbus;
            break;
        case MODE_ISO9141_5BAUD_INIT:
        case MODE_ISO14230_5BAUD_INIT:
        case MODE_ISO14230_FAST_INIT:
            gpio_output_set(GPIO_KLINE_EN, true);
            gpio_output_set(GPIO_ISO_K, true);
            _port = &kline;
            break;
        case MODE_J1850_PWM:
            gpio_output_set(GPIO_SAE_PWM, true);
            gpio_output_set(GPIO_J1850_TX, false);
            break;
        case MODE_J1850_VPW:
            gpio_output_set(GPIO_SAE_PWM, false);
            gpio_output_set(GPIO_J1850_TX, false);
            break;
        default:
            _port = 0;
            break;
    }

    if (_port) {
        _mode = mode;
        _port->setMode(_mode);
    }
}

void OBD2::disable(void)
{
    if (_port) {
        _port->setMode(MODE_IDLE);
        _port = 0;
    }

    operation_mode_t mode = _mode;
    _mode = MODE_IDLE;    

    switch (mode) {
        case MODE_HS_CAN:
        case MODE_MS_CAN:
        case MODE_SW_CAN:
            gpio_output_set(GPIO_CAN_EN, false);
            gpio_output_set(GPIO_CAN_SEL0, false);
            gpio_output_set(GPIO_CAN_SEL1, false);
            break;
        case MODE_ISO9141_5BAUD_INIT:
        case MODE_ISO14230_5BAUD_INIT:
        case MODE_ISO14230_FAST_INIT:
            gpio_output_set(GPIO_KLINE_EN, false);
            break;
        case MODE_J1850_PWM:
        case MODE_J1850_VPW:
            gpio_output_set(GPIO_J1850_TX, false);
            break;
        default:
            break;
    }
}

operation_mode_t OBD2::scan(int delay_ms)
{
    operation_mode_t scan_mode = MODE_HS_CAN;
    while (1) {
        setMode(scan_mode);
        k_sleep(K_MSEC(delay_ms));
        if (1) {    // active
            return scan_mode;
        }

        // Stupid C++. :)
        int mode = static_cast<int>(scan_mode);
        if (++mode >= MAX_MODE) {
            scan_mode = MODE_HS_CAN;
        } else {
            scan_mode = static_cast<operation_mode_t>(mode);
        }
    }
}

void OBD2::tx_thread(void)
{
    obd_packet_t packet;
    int status;

    while (1) {
        status = k_msgq_get(&obd2_tx_msgq, &packet, K_FOREVER);
        if (status != 0) {
            continue;
        }

        if (_port) {
            _port->send(&packet);
        }
    }
}

void OBD2::rx_thread(void)
{
    obd_packet_t packet;
    int status;

    while (1) {
        status = k_msgq_get(&obd2_rx_msgq, &packet, K_FOREVER);
        if (status != 0) {
            continue;
        }

        // do something

    }
}

void obd2_init(void)
{
    obd2.begin();
}

void set_operation_mode(operation_mode_t mode)
{
    obd2.setMode(mode);
}

operation_mode_t get_operation_mode(void)
{
    return obd2.getMode();
}

operation_mode_t scan_operation_modes(int delay_ms)
{
    return obd2.scan(delay_ms);
}

void obd2_rx_thread(void *arg1, void *arg2, void *arg3) 
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	obd2.rx_thread();
}

void obd2_tx_thread(void *arg1, void *arg2, void *arg3) 
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	obd2.tx_thread();
}

