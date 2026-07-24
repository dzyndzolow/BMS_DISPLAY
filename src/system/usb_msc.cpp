/**
 * @file usb_msc.cpp
 * @brief USB Mass Storage implementation for SD Card
 */

#include <usb_msc.h>
#include <USB.h>
#include <USBMSC.h>
#include <SD.h>
#include <logger.h>

static const char* TAG = "USB_MSC";
static USBMSC msc;
static bool mscActive = false;
static bool mscInitialized = false;

// Mutex for SD card access synchronization
static SemaphoreHandle_t sdMutex = nullptr;

/**
 * @brief Callback for reading from SD card
 */
static int32_t onRead(uint32_t lba, uint32_t offset, void* buffer, uint32_t bufsize) {
    if (xSemaphoreTake(sdMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        // Read sector by sector
        bool success = true;
        uint32_t sectors = (bufsize + 511) / 512;  // Round up to sector count
        
        for (uint32_t i = 0; i < sectors && success; i++) {
            success = SD.readRAW((uint8_t*)buffer + (i * 512), lba + i);
        }
        
        xSemaphoreGive(sdMutex);
        return success ? bufsize : -1;
    }
    return -1;
}

/**
 * @brief Callback for writing to SD card
 */
static int32_t onWrite(uint32_t lba, uint32_t offset, uint8_t* buffer, uint32_t bufsize) {
    if (xSemaphoreTake(sdMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        // Write sector by sector
        bool success = true;
        uint32_t sectors = (bufsize + 511) / 512;  // Round up to sector count
        
        for (uint32_t i = 0; i < sectors && success; i++) {
            success = SD.writeRAW((uint8_t*)buffer + (i * 512), lba + i);
        }
        
        xSemaphoreGive(sdMutex);
        return success ? bufsize : -1;
    }
    return -1;
}

/**
 * @brief Callback when USB is started/stopped
 */
static bool onStartStop(uint8_t power_condition, bool start, bool load_eject) {
    if (start) {
        Serial.println("[USB_MSC] Host mounted SD card");
        log_info(TAG, "USB Mass Storage mounted by host");
    } else {
        Serial.println("[USB_MSC] Host unmounted SD card");
        log_info(TAG, "USB Mass Storage unmounted by host");
    }
    return true;
}

bool usb_msc_init() {
    if (mscInitialized) {
        Serial.println("[USB_MSC] Already initialized");
        return true;
    }
    
    // Create mutex for SD access
    if (sdMutex == nullptr) {
        sdMutex = xSemaphoreCreateMutex();
        if (sdMutex == nullptr) {
            Serial.println("[USB_MSC] ERROR: Failed to create mutex");
            return false;
        }
    }
    
    // Check if SD card is available
    if (!SD.begin()) {
        Serial.println("[USB_MSC] ERROR: SD card not available");
        return false;
    }
    
    uint64_t cardSize = SD.cardSize();
    if (cardSize == 0) {
        Serial.println("[USB_MSC] ERROR: Invalid SD card size");
        return false;
    }
    
    Serial.println("[USB_MSC] Initializing USB Mass Storage...");
    Serial.printf("[USB_MSC] SD Card Size: %llu MB\n", cardSize / (1024 * 1024));
    
    // Configure USB MSC device
    msc.vendorID("ESP32");
    msc.productID("S3-SD-Card");
    msc.productRevision("1.0");
    msc.onRead(onRead);
    msc.onWrite(onWrite);
    msc.onStartStop(onStartStop);
    msc.mediaPresent(true);
    
    // Begin USB MSC with card size and sector size (512 bytes)
    if (!msc.begin(cardSize / 512, 512)) {
        Serial.println("[USB_MSC] ERROR: Failed to start MSC");
        return false;
    }
    
    // Start composite USB (CDC + MSC)
    USB.begin();
    
    mscInitialized = true;
    Serial.println("[USB_MSC] Initialized successfully");
    Serial.println("[USB_MSC] SD Card will appear as removable drive");
    log_info(TAG, "USB Mass Storage initialized");
    
    return true;
}

void usb_msc_start() {
    if (!mscInitialized) {
        if (!usb_msc_init()) {
            return;
        }
    }
    
    mscActive = true;
    Serial.println("[USB_MSC] Started");
}

void usb_msc_stop() {
    if (!mscActive) {
        return;
    }
    
    mscActive = false;
    Serial.println("[USB_MSC] Stopped");
}

bool usb_msc_is_active() {
    return mscActive;
}

/**
 * @brief USB MSC maintenance task
 * 
 * Low-priority task that handles USB MSC events and maintenance.
 * Runs with minimal CPU usage.
 */
void usbMscTask(void* parameter) {
    const TickType_t updateInterval = pdMS_TO_TICKS(100); // 100ms interval
    TickType_t lastUpdate = xTaskGetTickCount();
    
    Serial.println("[USB_MSC_Task] Started on Core 1");
    log_info(TAG, "USB MSC task started");
    
    // Wait a bit before initializing to ensure SD is ready
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    // Initialize USB MSC
    if (!usb_msc_init()) {
        Serial.println("[USB_MSC_Task] Initialization failed, task exiting");
        vTaskDelete(NULL);
        return;
    }
    
    usb_msc_start();
    
    while (1) {
        // Wait for next update cycle
        vTaskDelayUntil(&lastUpdate, updateInterval);
        
        // Minimal maintenance - just keep alive
        // The actual USB handling is done by callbacks
        if (mscActive) {
            // Optional: Add periodic status check here
            // For now, just yield CPU to other tasks
            taskYIELD();
        }
    }
}
