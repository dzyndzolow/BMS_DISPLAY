/*FreeRTOS task definitions*/
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/*LVGL rendering and input processing task - runs on Core 0*/
void lvglTask(void *pvParameters);

/*Default system task - runs on Core 1*/
void defaultTask(void *pvParameters);

#ifdef __cplusplus
}
#endif
