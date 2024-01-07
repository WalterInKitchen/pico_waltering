#include "hal_input.h"
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"

static hal_key_input_callback keyCallback;

typedef struct
{
    uint gpio;
    hal_input_key key;
} gpioKeyDefine;

// GPIO与KEY值的映射关系
static gpioKeyDefine keysDefine[] = {
    {14, KEY_UP},
    {13, KEY_OK},
    {12, KEY_DOWN},
    {11, KEY_CANCEL},
};

#define keysDefineLength sizeof(keysDefine) / sizeof(gpioKeyDefine)

void gpio_callback(uint gpio, uint32_t events)
{
    uint i = 0;
    bool state;

    if (!keyCallback)
    {
        return;
    }
    state = gpio_get(gpio);

    for (i = 0; i < keysDefineLength; i++)
    {
        if (keysDefine[i].gpio != gpio)
        {
            continue;
        }
        (*keyCallback)(keysDefine[i].key, state == true ? RELEASED : PRESSED);
        break;
    }
}

void hal_input_init(hal_key_input_callback callback)
{
    uint i = 0;
    keyCallback = callback;

    for (i = 0; i < keysDefineLength; i++)
    {
        gpio_set_pulls(keysDefine[i].gpio, true, false);
        gpio_set_irq_enabled_with_callback(keysDefine[i].gpio, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &gpio_callback);
    }
}
