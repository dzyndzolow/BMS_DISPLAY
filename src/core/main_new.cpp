/**
 * @file main_new.cpp
 * @brief New Main Entry Point - Clean Architecture
 * 
 * This file replaces LvglWidgets.cpp as the main entry point.
 * Uses unified boot sequence and clean task management.
 * 
 * @version 1.0.0 (MASTER_PLAN Implementation)
 * 
 * MIGRATION: To switch to new architecture:
 * 1. Rename LvglWidgets.cpp to LvglWidgets.cpp.bak
 * 2. Rename this file to main.cpp OR
 * 3. Add build flag: -D USE_NEW_ARCHITECTURE
 */

#ifdef USE_NEW_ARCHITECTURE

#include <Arduino.h>
#include "core/boot.h"
#include "core/tasks_new.h"
#include "config/defaults.h"

void setup() {
    /* Run unified boot sequence */
    Core::BootResult result = Core::boot();
    
    if (!result.success) {
        Serial.printf("BOOT FAILED at stage: %s\n", 
            Core::getBootStageName(result.failedStage));
        Serial.printf("Error: %s\n", result.errorMessage);
        Core::emergencyShutdown(result.errorMessage);
        return;
    }
    
    /* Create FreeRTOS tasks */
    if (!Core::createTasks()) {
        Core::emergencyShutdown("Failed to create tasks");
        return;
    }
    
    Serial.println("[MAIN] System running, tasks scheduled");
}

void loop() {
    /* FreeRTOS tasks handle everything */
    /* Delete this task to free resources */
    vTaskDelete(NULL);
}

#endif /* USE_NEW_ARCHITECTURE */
