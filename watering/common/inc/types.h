#ifndef _TYPES_H_
#define _TYPES_H_

#define SECONDS_TO_MS(SEC) (SEC * 1000)
#define SECONDS_TO_TICK(SEC) (pdMS_TO_TICKS(SECONDS_TO_MS(SEC)))
#define MINUTE_TO_TICK(MIN) (SECONDS_TO_TICK(MIN * 60))

typedef enum
{
    eFalse = 0,
    eTrue
} eBool;

typedef unsigned int uint32;

#endif
