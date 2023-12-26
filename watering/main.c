#include <stdio.h>
#include "pico/stdlib.h"
#include "log.h"

int main()
{
    uint32_t count = 0;
    stdio_init_all();
    while (true)
    {
        log_error("Hello, world heihei %d!", ++count);
        sleep_ms(1000);
    }
}
