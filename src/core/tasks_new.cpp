/**
 * @file tasks_new.cpp
 * @brief FreeRTOS Task Implementations - New Architecture
 * 
 * @version 1.0.0 (MASTER_PLAN Phase 2)
 */

#include "tasks_new.h"
#include "boot.h"
#include "../config/defaults.h"
#include "../services/event_bus.h"
#include <Arduino.h>
#include <lvgl.h>

namespace Core {

/* Task handles */
static TaskHandle_t s_lvglTaskHandle = nullptr;
static TaskHandle_t s_systemTaskHandle = nullptr;

/* Forward declarations */
extern void lvgl_handler();

/*===========================================================================*/
/*                             LVGL TASK                                      */
/*===========================================================================*/

void lvglTaskNew(void* pvParameters) {
    (void)pvParameters;
    
    Serial.println("[Task:LVGL] Started on core " + String(xPortGetCoreID()));
    
    /* Publish boot complete event */
    Services::EventBus::instance().publish(Services::EventType::SYSTEM_BOOT_COMPLETE);
    
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xPeriod = pdMS_TO_TICKS(TaskConfig::LVGL_TICK_MS);
    
    while (true) {
        /* Handle LVGL tasks */
        lvgl_handler();
        
        /* Precise timing */
        vTaskDelayUntil(&xLastWakeTime, xPeriod);
    }
}

/*===========================================================================*/
/*                            SYSTEM TASK                                     */
/*===========================================================================*/

void systemTask(void* pvParameters) {
    (void)pvParameters;
    
    Serial.println("[Task:System] Started on core " + String(xPortGetCoreID()));
    
    uint32_t loopCount = 0;
    
    while (true) {
        /* Periodic system maintenance */
        loopCount++;
        
        /* Every 10 seconds: memory check */
        if (loopCount % 100 == 0) {  /* 100 * 100ms = 10s */
            uint32_t freeHeap = esp_get_free_heap_size();
            uint32_t minFreeHeap = esp_get_minimum_free_heap_size();
            
            if (Features::DEBUG_MEMORY_USAGE) {
                Serial.printf("[Task:System] Heap: %lu free, %lu min\n",
                    (unsigned long)freeHeap, (unsigned long)minFreeHeap);
            }
            
            /* Publish low memory warning if needed */
            if (freeHeap < 50000) {
                Services::EventBus::instance().publishInt(
                    Services::EventType::SYSTEM_LOW_MEMORY,
                    freeHeap
                );
            }
        }
        
        /* Yield to other tasks */
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

/*===========================================================================*/
/*                            TASK CREATION                                   */
/*===========================================================================*/

bool createTasks() {
    BaseType_t result;
    
    /* Create LVGL task */
    result = xTaskCreatePinnedToCore(
        lvglTaskNew,
        "LVGL",
        TaskConfig::LVGL_STACK_SIZE,
        nullptr,
        TaskConfig::LVGL_PRIORITY,
        &s_lvglTaskHandle,
        TaskConfig::LVGL_CORE
    );
    
    if (result != pdPASS) {
        Serial.println("[Tasks] Failed to create LVGL task!");
        return false;
    }
    
    /* Create System task */
    result = xTaskCreatePinnedToCore(
        systemTask,
        "System",
        TaskConfig::SYSTEM_STACK_SIZE,
        nullptr,
        TaskConfig::SYSTEM_PRIORITY,
        &s_systemTaskHandle,
        TaskConfig::SYSTEM_CORE
    );
    
    if (result != pdPASS) {
        Serial.println("[Tasks] Failed to create System task!");
        return false;
    }
    
    Serial.println("[Tasks] All tasks created successfully");
    return true;
}

TaskHandle_t getLvglTaskHandle() {
    return s_lvglTaskHandle;
}

TaskHandle_t getSystemTaskHandle() {
    return s_systemTaskHandle;
}

} // namespace Core
