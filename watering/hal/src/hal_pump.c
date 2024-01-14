#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hal_pump.h"

#define PUMP_OUT_PORT 10
#define LED_OUT_PORT 25

void hal_pump_init(void)
{
    gpio_init(PUMP_OUT_PORT);
    gpio_set_dir(PUMP_OUT_PORT, GPIO_OUT);
    gpio_set_pulls(PUMP_OUT_PORT, false, true);

    gpio_init(LED_OUT_PORT);
    gpio_set_dir(LED_OUT_PORT, GPIO_OUT);
    gpio_set_pulls(LED_OUT_PORT, false, true);
    hal_pump_control(eFalse);
}

void hal_pump_control(eBool state)
{
    if (state)
    {
        gpio_put(PUMP_OUT_PORT, true);
        gpio_put(LED_OUT_PORT, true);
        return;
    }
    gpio_put(PUMP_OUT_PORT, false);
    gpio_put(LED_OUT_PORT, false);
}