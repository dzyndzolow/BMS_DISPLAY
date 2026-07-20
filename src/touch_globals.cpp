/*Touch module globals - define touch variables in single compilation unit*/
#include "touch.h"
#if defined(TOUCH_MODULE_ADDR)
#include <Touch_GT911.h>
Touch_GT911 touch(TOUCH_SDA, TOUCH_SCL, TOUCH_INT, TOUCH_RES, SCREEN_WIDTH, SCREEN_HEIGHT);
#endif


/*Touch calibration globals*/
bool touch_swap_xy = false;
int16_t touch_map_x1 = -1;
int16_t touch_map_x2 = -1;
int16_t touch_map_y1 = -1;
int16_t touch_map_y2 = -1;

int16_t touch_max_x = 0, touch_max_y = 0;
int16_t touch_raw_x = 0, touch_raw_y = 0;
int16_t touch_last_x = 0, touch_last_y = 0;
