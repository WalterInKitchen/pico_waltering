#ifndef _DISPLAY_OLED_H_
#define _DISPLAY_OLED_H_

#include "types.h"

void display_oled_init(void);

void display_highlight_settings(uint32);

void display_refresh_content(void);

void display_enter_power_save(void);

void display_exit_power_save(void);

#endif
