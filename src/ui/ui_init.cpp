/*LVGL UI initialization and management*/
#include "ui_init.h"
#include <lvgl.h>
#include <lv_demo_widgets.h>
#include <display.h>
#include <touch.h>
#include <touch_input.h>

/*LVGL globals*/
static lv_disp_draw_buf_t draw_buf;
static lv_color_t *disp_draw_buf = NULL;
static uint32_t buf_size = 0;

/*Initialize LVGL and display driver*/
bool ui_init(void) {
    uint32_t width = display_get_width();
    uint32_t height = display_get_height();
    
    /*#define DIRECT_MODE for full frame, else use Canvas*/
    #ifdef DIRECT_MODE
    buf_size = width * height;
    #else
    buf_size = width * 40;
    #endif
    
    /*Allocate draw buffer*/
    #ifdef ESP32
    disp_draw_buf = (lv_color_t *)heap_caps_malloc(sizeof(lv_color_t) * buf_size, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    if (!disp_draw_buf) {
        disp_draw_buf = (lv_color_t *)heap_caps_malloc(sizeof(lv_color_t) * buf_size, MALLOC_CAP_8BIT);
    }
    #else
    disp_draw_buf = (lv_color_t *)malloc(sizeof(lv_color_t) * buf_size);
    #endif
    
    if (!disp_draw_buf) {
        Serial.println("[UI] Draw buffer allocation failed!");
        return false;
    }
    
    /*Initialize LVGL*/
    lv_init();
    
    /*Setup display driver*/
    lv_disp_draw_buf_init(&draw_buf, disp_draw_buf, NULL, buf_size);
    lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = width;
    disp_drv.ver_res = height;
    disp_drv.flush_cb = display_flush_cb;
    disp_drv.draw_buf = &draw_buf;
    #ifdef DIRECT_MODE
    disp_drv.direct_mode = true;
    #endif
    lv_disp_drv_register(&disp_drv);
    
    /*Setup input driver (touch)*/
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = touch_input_cb;
    lv_indev_drv_register(&indev_drv);
    
    Serial.println("[UI] LVGL initialized");
    return true;
}

/*Start UI demo (create screens/tabs)*/
bool ui_start_demo(void) {
    /*Call LVGL demo widgets (creates all tabs)*/
    lv_demo_widgets();
    Serial.println("[UI] Demo widgets started");
    return true;
}

/*Main LVGL tick handler - call in main loop*/
void ui_handler(void) {
    lv_timer_handler();
    
    #ifdef DIRECT_MODE
    #if (LV_COLOR_16_SWAP != 0)
    display_get_gfx()->draw16bitBeRGBBitmap(0, 0, (uint16_t *)disp_draw_buf, display_get_width(), display_get_height());
    #else
    display_get_gfx()->draw16bitRGBBitmap(0, 0, (uint16_t *)disp_draw_buf, display_get_width(), display_get_height());
    #endif
    #endif
    
    #ifdef CANVAS
    display_flush_frame();
    #endif
}
