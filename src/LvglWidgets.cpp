/*******************************************************************************
 * LVGL Widgets - Main Orchestration File
 * 
 * Refactored modular architecture:
 * - Hardware initialization: display.h, touch_input.h
 * - LVGL setup: ui_init.h
 * - System tasks: tasks.h
 * - Utilities: sd_card.h, logger.h
 * 
 * This file only contains setup() and loop() orchestration
 ******************************************************************************/

#include <Arduino.h>
#include <tasks.h>
#include <sd_card.h>
#include <logger.h>

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n\n=== ESP32-S3 LVGL Widgets System ===");
  
  /*Initialize SD card and logger*/
  if (sd_card_init()) {
    Serial.println("[MAIN] SD Card initialized successfully");
    logger_init();
    log_info("MAIN", "System started, SD and logger initialized");
  } else {
    Serial.println("[MAIN] SD Card init failed - logging to Serial only");
  }
  
  /*Create FreeRTOS tasks*/
  xTaskCreatePinnedToCore(lvglTask, "LVGL Task", 8192, NULL, 2, NULL, 0);
  xTaskCreatePinnedToCore(defaultTask, "Default Task", 4096, NULL, 1, NULL, 1);
  
  Serial.println("[MAIN] Tasks created, starting scheduler");
}

void loop() {
  /*Tasks have taken over control, loop not used*/
  vTaskDelete(NULL);
}
