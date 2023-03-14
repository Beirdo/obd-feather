#ifndef __GPIO_MAP_H_
#define __GPIO_MAP_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <device.h>
#include <drivers/gpio.h>

extern struct gpio_dt_spec gpio_input_specs[];
extern int gpio_input_count;

extern struct gpio_dt_spec gpio_output_specs[];
extern int gpio_output_count;

typedef enum {
    GPIO_J1850_RX = 0,
} gpio_input_t;

typedef enum {
    GPIO_J1850_TX = 0,
    GPIO_SAE_PWM,
    GPIO_CAN_EN,
    GPIO_CAN_SEL0,
    GPIO_CAN_SEL1,
    GPIO_SW_CAN_MODE0,
    GPIO_SW_CAN_MODE1,
    GPIO_SW_CAN_LOAD,
    GPIO_KLINE_EN,
    GPIO_ISO_K,
    GPIO_NEOPIXEL,
} gpio_output_t;

void gpio_init(void);
void gpio_output_set(int index, int value);

#ifdef __cplusplus
}
#endif

#endif