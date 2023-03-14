#include <zephyr.h>

#include "gpio_map.h"
#include "modes.h"

operation_mode_t current_mode = MODE_IDLE;

K_MUTEX_DEFINE(mode_mutex);

void disable_mode(operation_mode_t mode);
void enable_mode(operation_mode_t mode);

void set_operation_mode(operation_mode_t mode)
{
    if (mode < 0 || mode >= MAX_MODE) {
        return;
    }

    k_mutex_lock(&mode_mutex, K_FOREVER);
    
    disable_mode(current_mode);
    enable_mode(mode);
    current_mode = mode;

    k_mutex_unlock(&mode_mutex);
}

operation_mode_t get_operation_mode(void)
{
    operation_mode_t mode;
    
    k_mutex_lock(&mode_mutex, K_FOREVER);
    mode = current_mode;
    k_mutex_unlock(&mode_mutex);

    return mode;
}

operation_mode_t scan_operation_modes(int ms_delay)
{
    operation_mode_t scan_mode = MODE_HS_CAN;
    while (1) {
        set_operation_mode(scan_mode);
        k_sleep(K_MSEC(ms_delay));
        if (1) {    // active
            return scan_mode;
        }

        scan_mode++;
        if (scan_mode >= MAX_MODE) {
            scan_mode = MODE_HS_CAN;
        }
    }
}

void disable_mode(operation_mode_t mode)
{
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

void enable_mode(operation_mode_t mode)
{
    switch (mode) {
        case MODE_HS_CAN:
            gpio_output_set(GPIO_CAN_SEL0, GPIO_OUTPUT_INACTIVE);
            gpio_output_set(GPIO_CAN_SEL1, GPIO_OUTPUT_ACTIVE);
            gpio_output_set(GPIO_CAN_EN, GPIO_OUTPUT_ACTIVE);
            break;
        case MODE_MS_CAN:
            gpio_output_set(GPIO_CAN_SEL0, GPIO_OUTPUT_ACTIVE);
            gpio_output_set(GPIO_CAN_SEL1, GPIO_OUTPUT_INACTIVE);
            gpio_output_set(GPIO_CAN_EN, GPIO_OUTPUT_ACTIVE);
            break;
        case MODE_SW_CAN:
            gpio_output_set(GPIO_CAN_SEL0, GPIO_OUTPUT_ACTIVE);
            gpio_output_set(GPIO_CAN_SEL1, GPIO_OUTPUT_ACTIVE);
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
            break;
    }
}
