#include <device.h>
#include <drivers/gpio.h>
#include <stdlib.h>

#include "gpio_map.h"
#include "j1850.h"


struct gpio_dt_spec gpio_input_specs[] = {
    {   // J1850_RX
        .port = DEVICE_DT_GET(DT_NODELABEL(gpiob)),
        .pin = 11,
        .dt_flags = GPIO_ACTIVE_HIGH,
    },
};

const int gpio_input_count = sizeof(gpio_input_specs) / sizeof(gpio_input_specs[0]);

struct gpio_callback *gpio_input_callbacks;
gpio_callback_handler_t gpio_input_handlers[] = {
    j1850_rx_callback,
};

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

const int gpio_output_count = sizeof(gpio_output_specs) / sizeof(gpio_output_specs[0]);

void gpio_init(void)
{
    int i;

    gpio_input_callbacks = (struct gpio_callback *)malloc(sizeof(struct gpio_callback) * gpio_input_count);

    for (i = 0; i < gpio_input_count; i++) {
        struct gpio_dt_spec *spec = &gpio_input_specs[i];

        gpio_pin_configure_dt(spec, GPIO_INPUT);
        
        gpio_pin_interrupt_configure_dt(spec, GPIO_INT_EDGE_BOTH);
        gpio_init_callback(&gpio_input_callbacks[i], gpio_input_handlers[i], 0);
        gpio_add_callback(spec->port, &gpio_input_callbacks[i]);
    }

    for (i = 0; i < gpio_output_count; i++) {
        struct gpio_dt_spec *spec = &gpio_output_specs[i];

        gpio_pin_configure_dt(spec, GPIO_OUTPUT_INACTIVE);
    }
}

void gpio_output_set(int index, bool value)
{
    if (index < 0 || index >= gpio_output_count) {
        return;
    }

    struct gpio_dt_spec *spec = &gpio_output_specs[index];
    gpio_pin_set_dt(spec, value);
}

bool gpio_input_get(int index)
{
    if (index < 0 || index >= gpio_input_count) {
        return false;
    }

    struct gpio_dt_spec *spec = &gpio_input_specs[index];
    return gpio_pin_get_dt(spec);
}

void gpio_irq_enable(int index)
{
    if (index < 0 || index >= gpio_input_count) {
        return;
    }

    struct gpio_dt_spec *spec = &gpio_input_specs[index];
    gpio_input_callbacks[index].pin_mask = 1 << spec->pin;
}

void gpio_irq_disable(int index)
{
    if (index < 0 || index >= gpio_input_count) {
        return;
    }

    gpio_input_callbacks[index].pin_mask = 0;
}