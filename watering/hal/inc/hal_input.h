#ifndef _HAL_INPUT_H_
#define _HAL_INPUT_H_

#include "types.h"

typedef enum
{
    KEY_LEFT = 1,
    KEY_UP,
    KEY_RIGHT,
    KEY_DOWN,
    KEY_MIDDLE
} hal_input_key;

typedef enum
{
    RELEASED = 0,
    PRESSED
} hal_input_key_state;

typedef void (*hal_key_input_callback)(hal_input_key, hal_input_key_state);

/**
 * 初始化
 */
void hal_input_init(hal_key_input_callback);

#endif
