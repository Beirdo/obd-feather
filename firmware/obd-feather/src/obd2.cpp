#include <zephyr.h>

#include "obd2.h"
#include "gpio_map.h"
#include "modes.h"
#include "canbus.h"

OBD2 obd2;

void OBD2::begin(void)
{
    _port = 0;
    _mode = MODE_IDLE;
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

bool OBD2::send(uint32_t id, uint8_t pid, uint8_t *data, uint8_t len)
{
    if (_port) {
        return _port->send(id, pid, data, len);
    }
    return false;
}

bool OBD2::receive(uint32_t *id, uint8_t *pid, uint8_t *data, uint8_t *len)
{
    if (_port) {
        return _port->receive(id, pid, data, len);
    }
    return false;
}

void OBD2::enable(operation_mode_t mode)
{
    switch (mode) {
        case MODE_HS_CAN:
            gpio_output_set(GPIO_CAN_SEL0, GPIO_OUTPUT_INACTIVE);
            gpio_output_set(GPIO_CAN_SEL1, GPIO_OUTPUT_ACTIVE);
            gpio_output_set(GPIO_CAN_EN, GPIO_OUTPUT_ACTIVE);
            _port = &canbus;
            break;
        case MODE_MS_CAN:
            gpio_output_set(GPIO_CAN_SEL0, GPIO_OUTPUT_ACTIVE);
            gpio_output_set(GPIO_CAN_SEL1, GPIO_OUTPUT_INACTIVE);
            gpio_output_set(GPIO_CAN_EN, GPIO_OUTPUT_ACTIVE);
            _port = &canbus;
            break;
        case MODE_SW_CAN:
            gpio_output_set(GPIO_CAN_SEL0, GPIO_OUTPUT_ACTIVE);
            gpio_output_set(GPIO_CAN_SEL1, GPIO_OUTPUT_ACTIVE);
            _port = &canbus;
            break;
        case MODE_K_LINE:
            gpio_output_set(GPIO_KLINE_EN, GPIO_OUTPUT_ACTIVE);
            gpio_output_set(GPIO_ISO_K, GPIO_OUTPUT_ACTIVE);
            break;
        case MODE_J1850_PWM:
        case MODE_J1850_VPW:
            gpio_output_set(GPIO_J1850_TX, GPIO_OUTPUT_INACTIVE);
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
            gpio_output_set(GPIO_CAN_EN, GPIO_OUTPUT_INACTIVE);
            gpio_output_set(GPIO_CAN_SEL0, GPIO_OUTPUT_INACTIVE);
            gpio_output_set(GPIO_CAN_SEL1, GPIO_OUTPUT_INACTIVE);
            break;
        case MODE_K_LINE:
            gpio_output_set(GPIO_KLINE_EN, GPIO_OUTPUT_INACTIVE);
            break;
        case MODE_J1850_PWM:
        case MODE_J1850_VPW:
            gpio_output_set(GPIO_J1850_TX, GPIO_OUTPUT_INACTIVE);
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
