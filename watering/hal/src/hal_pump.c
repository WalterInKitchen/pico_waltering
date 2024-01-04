#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hal_pump.h"

#define PUMP_OUT_PORT 10

void hal_pump_init(void)
{
    gpio_set_pulls(PUMP_OUT_PORT, false, true);
    hal_pump_control(eFalse);
}

void hal_pump_control(eBool state)
{
    if (state)
    {
        gpio_put(PUMP_OUT_PORT, true);
        return;
    }
    gpio_put(PUMP_OUT_PORT, false);
}