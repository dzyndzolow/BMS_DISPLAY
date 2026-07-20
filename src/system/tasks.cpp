/*FreeRTOS task definitions for LVGL, input, and system management*/
#include "tasks.h"
#include "../ui/screens/battery_screen_v1.h"
#include "../ui/screens/battery_screen_v2.h"
#include "../ui/screens/screen_manager.h"
#include "touch.h"
#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include <HardwareSerial.h>
#include <lv_demo_widgets.h>


/*Using Arduino_Canvas for NV3041A, ensure flush is called*/
#define CANVAS 1

#define GFX_BL 1
const uint16_t BLACK = 0xFFFF;

/*Global GFX instance*/
static Arduino_DataBus *bus = nullptr;
static Arduino_GFX *g = nullptr;
static Arduino_GFX *gfx = nullptr;

/*Display globals for old compatibility*/
static uint32_t screenWidth;
static uint32_t screenHeight;
static uint32_t bufSize;
static lv_disp_draw_buf_t draw_buf;
static lv_color_t *disp_draw_buf = nullptr;
static lv_disp_drv_t disp_drv;

/*Old display flush callback (kept for compatibility with original UI code)*/
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area,
                   lv_color_t *color_p) {
#ifndef DIRECT_MODE
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);
#if (LV_COLOR_16_SWAP != 0)
  gfx->draw16bitBeRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w,
                            h);
#else
  gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);
#endif
#endif
  lv_disp_flush_ready(disp);
}

/*Old touch read callback (kept for compatibility)*/
void my_touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data) {
  if (touch_has_signal()) {
    if (touch_touched()) {
      data->state = LV_INDEV_STATE_PR;
      data->point.x = screenWidth - touch_last_x;
      data->point.y = screenHeight - touch_last_y;
    } else if (touch_released()) {
      data->state = LV_INDEV_STATE_REL;
    }
  } else {
    data->state = LV_INDEV_STATE_REL;
  }
}

/*Initialize GFX hardware*/
static bool init_gfx_hardware() {
  /*Create QSPI bus and NV3041A display*/
  bus = new Arduino_ESP32QSPI(45 /* cs */, 47 /* sck */, 21 /* d0 */,
                              48 /* d1 */, 40 /* d2 */, 39 /* d3 */);
  g = new Arduino_NV3041A(bus, GFX_NOT_DEFINED /* RST */, 0 /* rotation */,
                          true /* IPS */);
  gfx = new Arduino_Canvas(480 /* width */, 272 /* height */, g);

  if (!gfx->begin()) {
    Serial.println("[Tasks] GFX init failed!");
    return false;
  }

  gfx->fillScreen(BLACK);

  /*Setup backlight PWM*/
  ledcAttach(GFX_BL, 20000, 8);
  ledcWrite(GFX_BL, 255);

  Serial.println("[Tasks] GFX initialized successfully");
  return true;
}

/*Initialize LVGL with display and input drivers*/
static bool init_lvgl() {
  screenWidth = gfx->width();
  screenHeight = gfx->height();

#ifdef DIRECT_MODE
  bufSize = screenWidth * screenHeight;
#else
  bufSize = screenWidth * 40;
#endif

  /*Allocate draw buffer*/
#ifdef ESP32
  disp_draw_buf = (lv_color_t *)heap_caps_malloc(
      sizeof(lv_color_t) * bufSize, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  if (!disp_draw_buf) {
    disp_draw_buf = (lv_color_t *)heap_caps_malloc(sizeof(lv_color_t) * bufSize,
                                                   MALLOC_CAP_8BIT);
  }
#else
  disp_draw_buf = (lv_color_t *)malloc(sizeof(lv_color_t) * bufSize);
#endif

  if (!disp_draw_buf) {
    Serial.println("[Tasks] Draw buffer allocation failed!");
    return false;
  }

  /*Initialize LVGL*/
  lv_init();

  /*Setup display driver*/
  lv_disp_draw_buf_init(&draw_buf, disp_draw_buf, NULL, bufSize);
  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = screenWidth;
  disp_drv.ver_res = screenHeight;
  disp_drv.flush_cb = my_disp_flush;
  disp_drv.draw_buf = &draw_buf;
#ifdef DIRECT_MODE
  disp_drv.direct_mode = true;
#endif
  lv_disp_drv_register(&disp_drv);

  /*Setup input driver*/
  static lv_indev_drv_t indev_drv;
  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = my_touchpad_read;
  lv_indev_drv_register(&indev_drv);

  Serial.println("[Tasks] LVGL initialized successfully");
  return true;
}

/*Register and start battery monitor screens*/
static void init_battery_screens() {
  auto &mgr = UI::ScreenManager::instance();

  /* Register battery screens */
  mgr.registerScreen(new UI::BatteryScreenV1());
  mgr.registerScreen(new UI::BatteryScreenV2());

  /* Navigate to Battery V1 as initial screen */
  mgr.navigateTo(UI::ScreenId::BATTERY_V1, UI::ScreenTransition::NONE, false);

  Serial.println("[Tasks] Battery screens registered and started");
}

/*LVGL task - handles UI rendering and input processing*/
void lvglTask(void *pvParameters) {
  /*Initialize GFX hardware*/
  if (!init_gfx_hardware()) {
    Serial.println("[Tasks] GFX initialization failed!");
    vTaskDelete(NULL);
    return;
  }

  /*Initialize touch*/
  touch_init(gfx->width(), gfx->height(), gfx->getRotation());

  /*Initialize LVGL*/
  if (!init_lvgl()) {
    Serial.println("[Tasks] LVGL initialization failed!");
    vTaskDelete(NULL);
    return;
  }

  /*Start battery monitor screens (replaces lv_demo_widgets)*/
  init_battery_screens();
  Serial.println("[Tasks] Battery monitor screens started");

  /*Main LVGL rendering loop*/
  while (true) {
    lv_timer_handler();

    /* Update active screen */
    UI::ScreenManager::instance().update();

#ifdef DIRECT_MODE
#if (LV_COLOR_16_SWAP != 0)
    gfx->draw16bitBeRGBBitmap(0, 0, (uint16_t *)disp_draw_buf, screenWidth,
                              screenHeight);
#else
    gfx->draw16bitRGBBitmap(0, 0, (uint16_t *)disp_draw_buf, screenWidth,
                            screenHeight);
#endif
#endif

#ifdef CANVAS
    gfx->flush();
#endif

    vTaskDelay(pdMS_TO_TICKS(5));
  }
}

/*Default system task - system monitoring, logging, etc*/
void defaultTask(void *pvParameters) {
  while (true) {
    /*System monitoring, periodic tasks, logging, etc*/
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

/*Backlight control: percent 0-100*/
extern "C" void backlight_set(uint8_t percent) {
  if (percent > 100)
    percent = 100;
  uint8_t duty = (uint8_t)((uint16_t)percent * 255 / 100);
  ledcWrite(GFX_BL, duty);
}
