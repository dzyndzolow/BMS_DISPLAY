/*UI initialization module - LVGL setup and demo startup*/
#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*Initialize LVGL, allocate buffers, register drivers*/
bool ui_init(void);

/*Start demo widgets (creates all screens/tabs)*/
bool ui_start_demo(void);

/*Main LVGL handler - call in main loop/task*/
void ui_handler(void);

#ifdef __cplusplus
}
#endif
