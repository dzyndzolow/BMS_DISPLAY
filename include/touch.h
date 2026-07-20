#ifndef TOUCH_H
#define TOUCH_H
#include <cstdint>


/*******************************************************************************
 * Touch configuration for GT911 capacitive touch panel
 * I2C interface
 ******************************************************************************/

#define TOUCH_MODULES_GT911
#define TOUCH_MODULE_ADDR GT911_SLAVE_ADDRESS1

// I2C pins for GT911
#define TOUCH_SCL 4
#define TOUCH_SDA 8
#define TOUCH_RES 38
#define TOUCH_INT 3

// Touch calibration (auto-calculated based on rotation)
extern bool touch_swap_xy;
extern int16_t touch_map_x1;
extern int16_t touch_map_x2;
extern int16_t touch_map_y1;
extern int16_t touch_map_y2;

extern int16_t touch_max_x, touch_max_y;
extern int16_t touch_raw_x, touch_raw_y;
extern int16_t touch_last_x, touch_last_y;

// Forward declare screen dimensions
#ifndef SCREEN_WIDTH
#define SCREEN_WIDTH 480
#define SCREEN_HEIGHT 272
#endif

#if defined(TOUCH_MODULE_ADDR)
#include <Wire.h>
#include <Touch_GT911.h>

// Create GT911 touch object
extern Touch_GT911 touch;
#endif

static inline void touch_init(int16_t w, int16_t h, uint8_t r)
{
  touch_max_x = w - 1;
  touch_max_y = h - 1;
  if (touch_map_x1 == -1)
  {
    switch (r)
    {
    case 3:
      touch_swap_xy = true;
      touch_map_x1 = touch_max_x;
      touch_map_x2 = 0;
      touch_map_y1 = 0;
      touch_map_y2 = touch_max_y;
      break;
    case 2:
      touch_swap_xy = false;
      touch_map_x1 = touch_max_x;
      touch_map_x2 = 0;
      touch_map_y1 = touch_max_y;
      touch_map_y2 = 0;
      break;
    case 1:
      touch_swap_xy = true;
      touch_map_x1 = 0;
      touch_map_x2 = touch_max_x;
      touch_map_y1 = touch_max_y;
      touch_map_y2 = 0;
      break;
    default: // case 0:
      touch_swap_xy = false;
      touch_map_x1 = 0;
      touch_map_x2 = touch_max_x;
      touch_map_y1 = 0;
      touch_map_y2 = touch_max_y;
      break;
    }
  }

#if defined(TOUCH_MODULE_ADDR)
  // Reset touchscreen
#if (TOUCH_RES > 0)
  pinMode(TOUCH_RES, OUTPUT);
  digitalWrite(TOUCH_RES, 0);
  delay(200);
  digitalWrite(TOUCH_RES, 1);
  delay(200);
#endif
  
  // Initialize GT911
  touch.begin(GT911_ADDR1);
  touch.setRotation(r == 2 ? ROTATION_INVERTED : ROTATION_NORMAL);
#endif
}

static inline bool touch_has_signal()
{
#if defined(TOUCH_MODULE_ADDR)
  return true;
#endif
  return false;
}

static inline void translate_touch_raw()
{
  if (touch_swap_xy)
  {
    touch_last_x = map(touch_raw_y, touch_map_x1, touch_map_x2, 0, touch_max_x);
    touch_last_y = map(touch_raw_x, touch_map_y1, touch_map_y2, 0, touch_max_y);
  }
  else
  {
    touch_last_x = map(touch_raw_x, touch_map_x1, touch_map_x2, 0, touch_max_x);
    touch_last_y = map(touch_raw_y, touch_map_y1, touch_map_y2, 0, touch_max_y);
  }
}

static inline bool touch_touched()
{
#if defined(TOUCH_MODULE_ADDR)
  touch.read();
  if (touch.isTouched && touch.touches > 0)
  {
    touch_raw_x = touch.points[0].x;
    touch_raw_y = touch.points[0].y;
    touch_last_x = touch_raw_x;
    touch_last_y = touch_raw_y;
    translate_touch_raw();
    return true;
  }
#endif
  return false;
}

static inline bool touch_released()
{
#if defined(TOUCH_MODULE_ADDR)
  return !touch.isTouched;
#endif
  return false;
}

#endif // TOUCH_H
