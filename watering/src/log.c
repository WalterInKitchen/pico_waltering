#include <stdarg.h>
#include <stdio.h>
#include "pico/stdlib.h"
#include "log.h"

void log_tag(const char *tag, const char *fmt, va_list args)
{
    printf("%s:", tag);
    vprintf(fmt, args);
    printf("\r\n");
}

void log_error(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    log_tag("ERROR", format, args);
    va_end(args);
}
