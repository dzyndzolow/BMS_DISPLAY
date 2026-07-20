/*Display initialization and configuration for ESP32-S3 + NV3041A*/
#include "display.h"
#include <lvgl.h>
#include <Arduino_GFX_Library.h>

/*Display globals*/
static Arduino_GFX *gfx = NULL;
static uint32_t display_width = 0;
static uint32_t display_height = 0;

/*Initialize display hardware*/
bool display_init(void) {
    /*Setup QSPI bus for NV3041A display*/
    Arduino_DataBus *bus = new Arduino_ESP32QSPI(
        45 /* cs */, 47 /* sck */, 21 /* d0 */, 48 /* d1 */, 40 /* d2 */, 39 /* d3 */);
    
    Arduino_GFX *g = new Arduino_NV3041A(bus, GFX_NOT_DEFINED /* RST */, 0 /* rotation */, true /* IPS */);
    gfx = new Arduino_Canvas(480 /* width */, 272 /* height */, g);
    
    display_width = gfx->width();
    display_height = gfx->height();
    
    if (!gfx->begin()) {
        Serial.println("[DISPLAY] gfx->begin() failed!");
        return false;
    }
    
    gfx->fillScreen(0xFFFF); /*BLACK in 16-bit mode*/
    Serial.println("[DISPLAY] Display initialized (480x272)");
    return true;
}

/*Setup backlight PWM*/
bool backlight_init(void) {
    const uint8_t BL_PIN = 1;
    ledcAttach(BL_PIN, 20000, 8); /*pin, freq, resolution*/
    ledcWrite(BL_PIN, 255); /*Full brightness initially*/
    Serial.println("[DISPLAY] Backlight initialized");
    return true;
}

/*Get display dimensions*/
uint32_t display_get_width(void) {
    return display_width;
}

uint32_t display_get_height(void) {
    return display_height;
}

/*Get GFX object for drivers*/
Arduino_GFX* display_get_gfx(void) {
    return gfx;
}

/*Display flush callback for LVGL*/
void display_flush_cb(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
    if (!gfx) return;
    
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);
    
#if (LV_COLOR_16_SWAP != 0)
    gfx->draw16bitBeRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);
#else
    gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);
#endif
    
    lv_disp_flush_ready(disp);
}

/*Flush full frame (for CANVAS mode)*/
void display_flush_frame(void) {
    if (gfx) {
        gfx->flush();
    }
}

/*Set backlight brightness (0-100%)*/
void display_set_backlight(uint8_t percent) {
    if (percent > 100) percent = 100;
    uint8_t duty = (uint8_t)((uint16_t)percent * 255 / 100);
    ledcWrite(1, duty); /*BL_PIN = 1*/
}
