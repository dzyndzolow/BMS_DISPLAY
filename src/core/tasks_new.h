/**
 * @file tasks_new.h
 * @brief FreeRTOS Task Definitions - New Architecture
 * 
 * Clean task definitions using HAL and unified boot.
 * 
 * @version 1.0.0 (MASTER_PLAN Phase 2)
 */

#ifndef CORE_TASKS_NEW_H
#define CORE_TASKS_NEW_H

#include "../config/defaults.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

namespace Core {

/**
 * @brief LVGL Task - handles UI rendering on core 0
 * 
 * Responsibilities:
 * - Call lv_timer_handler() periodically
 * - Flush display buffer
 * - Process touch input
 */
void lvglTaskNew(void* pvParameters);

/**
 * @brief System Task - handles background operations on core 1
 * 
 * Responsibilities:
 * - System monitoring
 * - SD card operations
 * - Logging
 * - Future: WiFi, OTA
 */
void systemTask(void* pvParameters);

/**
 * @brief Create all FreeRTOS tasks
 * @return true if all tasks created successfully
 */
bool createTasks();

/**
 * @brief Get LVGL task handle
 */
TaskHandle_t getLvglTaskHandle();

/**
 * @brief Get System task handle
 */
TaskHandle_t getSystemTaskHandle();

} // namespace Core

#endif /* CORE_TASKS_NEW_H */
