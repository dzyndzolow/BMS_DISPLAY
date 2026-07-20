/*Display initialization header*/
#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>
#include <stdbool.h>
#include <lvgl.h>

#ifdef __cplusplus
class Arduino_GFX;
extern "C" {
#endif

/*Initialize display hardware (QSPI NV3041A)*/
bool display_init(void);

/*Initialize backlight PWM*/
bool backlight_init(void);

/*Get display width*/
uint32_t display_get_width(void);

/*Get display height*/
uint32_t display_get_height(void);

/*Get GFX object pointer (C++ needed)*/
#ifdef __cplusplus
Arduino_GFX* display_get_gfx(void);
#endif

/*LVGL display flush callback*/
void display_flush_cb(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p);

/*Flush full frame (CANVAS mode)*/
void display_flush_frame(void);

/*Set backlight (0-100%)*/
void display_set_backlight(uint8_t percent);

#ifdef __cplusplus
}
#endif

#endif
