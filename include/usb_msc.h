/**
 * @file usb_msc.h
 * @brief USB Mass Storage Class implementation for SD Card
 * 
 * Exposes SD card as USB drive to host computer while maintaining
 * serial console functionality (composite USB device).
 */

#pragma once

#include <Arduino.h>

/**
 * @brief Initialize USB Mass Storage Device
 * @return true if initialization successful
 */
bool usb_msc_init();

/**
 * @brief Start USB MSC task (call after SD card is mounted)
 */
void usb_msc_start();

/**
 * @brief Stop USB MSC and unmount from host
 */
void usb_msc_stop();

/**
 * @brief Check if USB MSC is active
 * @return true if MSC is running
 */
bool usb_msc_is_active();

/**
 * @brief USB MSC update task (call from FreeRTOS task)
 * @param parameter Task parameter (unused)
 */
void usbMscTask(void* parameter);
