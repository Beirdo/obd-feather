#include <device.h>
#include <drivers/gpio.h>

#include "gpio_map.h"


struct gpio_dt_spec gpio_input_specs[] = {
    {   // J1850_RX
        .port = DEVICE_DT_GET(DT_NODELABEL(gpiob)),
        .pin = 11,
        .dt_flags = GPIO_ACTIVE_HIGH,
    },
};

int gpio_input_count = sizeof(gpio_input_specs) / sizeof(gpio_input_specs[0]);


struct gpio_dt_spec gpio_output_specs[] = {
    {   // J1850_TX
        .port = DEVICE_DT_GET(DT_NODELABEL(gpiob)),
        .pin = 10,
        .dt_flags = GPIO_ACTIVE_HIGH,
    },
    {   // SAE_PWM
        .port = DEVICE_DT_GET(DT_NODELABEL(external_gpio)),
        .pin = 2,
        .dt_flags = GPIO_ACTIVE_HIGH,
    },
    {   // CAN_EN
        .port = DEVICE_DT_GET(DT_NODELABEL(gpioc)),
        .pin = 3,
        .dt_flags = GPIO_ACTIVE_LOW,
    },
    {   // CAN_SEL0
        .port = DEVICE_DT_GET(DT_NODELABEL(external_gpio)),
        .pin = 7,
        .dt_flags = GPIO_ACTIVE_HIGH,
    },
    {   // CAN_SEL1
        .port = DEVICE_DT_GET(DT_NODELABEL(external_gpio)),
        .pin = 6,
        .dt_flags = GPIO_ACTIVE_HIGH,
    },
    {   // SW_CAN_MODE0
        .port = DEVICE_DT_GET(DT_NODELABEL(external_gpio)),
        .pin = 5,
        .dt_flags = GPIO_ACTIVE_HIGH,
    },
    {   // SW_CAN_MODE1
        .port = DEVICE_DT_GET(DT_NODELABEL(external_gpio)),
        .pin = 4,
        .dt_flags = GPIO_ACTIVE_HIGH,
    },
    {   // SW_CAN_LOAD
        .port = DEVICE_DT_GET(DT_NODELABEL(external_gpio)),
        .pin = 3,
        .dt_flags = GPIO_ACTIVE_HIGH,
    },
    {   // KLINE_EN
        .port = DEVICE_DT_GET(DT_NODELABEL(external_gpio)),
        .pin = 1,
        .dt_flags = GPIO_ACTIVE_HIGH,
    },
    {   // ISO_K
        .port = DEVICE_DT_GET(DT_NODELABEL(external_gpio)),
        .pin = 0,
        .dt_flags = GPIO_ACTIVE_HIGH,
    },
    {   // NEOPIXEL
        .port = DEVICE_DT_GET(DT_NODELABEL(gpioc)),
        .pin = 0,
        .dt_flags = GPIO_ACTIVE_HIGH,
    },
};

int gpio_output_count = sizeof(gpio_output_specs) / sizeof(gpio_output_specs[0]);


void gpio_init(void)
{
    int i;

    for (i = 0; i < gpio_input_count; i++) {
        struct gpio_dt_spec *spec = &gpio_input_specs[i];

        gpio_pin_configure_dt(spec, GPIO_INPUT);
    }

    for (i = 0; i < gpio_output_count; i++) {
        struct gpio_dt_spec *spec = &gpio_output_specs[i];

        gpio_pin_configure_dt(spec, GPIO_OUTPUT_INACTIVE);
    }
}

void gpio_output_set(int index, int value)
{
    if (index < 0 || index >= gpio_output_count) {
        return;
    }

    struct gpio_dt_spec *spec = &gpio_output_specs[index];
    gpio_pin_set_dt(spec, value);
}