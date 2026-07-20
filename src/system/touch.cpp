#include <lvgl.h>
/*Touch input initialization*/
#include "touch.h"
#include <display.h>


/*LVGL touch input callback*/
void touch_input_cb(lv_indev_drv_t *indev_driver, lv_indev_data_t *data) {
    if (touch_has_signal()) {
        if (touch_touched()) {
            data->state = LV_INDEV_STATE_PR;
            
            /*Invert X and Y axes*/
            data->point.x = display_get_width() - touch_last_x;
            data->point.y = display_get_height() - touch_last_y;
        } else if (touch_released()) {
            data->state = LV_INDEV_STATE_REL;
        }
    } else {
        data->state = LV_INDEV_STATE_REL;
    }
}
