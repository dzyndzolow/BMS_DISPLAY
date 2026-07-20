/*Touch input header*/
#ifndef TOUCH_INPUT_H
#define TOUCH_INPUT_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

/*LVGL touch input callback*/
void touch_input_cb(lv_indev_drv_t *indev_driver, lv_indev_data_t *data);

#ifdef __cplusplus
}
#endif

#endif
