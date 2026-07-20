/**
 * @file boot.cpp
 * @brief Unified Boot Sequence Implementation
 * 
 * Single point of hardware initialization. Replaces scattered init code
 * from tasks.cpp, display.cpp, and ui_init.cpp.
 * 
 * @version 1.0.0 (MASTER_PLAN Phase 2)
 */

#include "boot.h"
#include "../config/defaults.h"
#include "../hal/NV3041ADisplay.h"
#include "../hal/GT911Touch.h"
#include <Arduino.h>
#include <lvgl.h>
#include <lv_demo_widgets.h>
#include <sd_card.h>
#include <logger.h>

namespace Core {

/* Static instances - single point of ownership */
static HAL::NV3041ADisplay* s_display = nullptr;
static HAL::GT911Touch* s_touch = nullptr;

/* LVGL resources */
static lv_disp_draw_buf_t s_drawBuf;
static lv_color_t* s_dispDrawBuf = nullptr;
static lv_disp_drv_t s_dispDrv;
static lv_indev_drv_t s_indevDrv;

/* Boot state */
static BootStage s_currentStage = BootStage::START;
static bool s_booted = false;

/*===========================================================================*/
/*                          FORWARD DECLARATIONS                              */
/*===========================================================================*/

static bool initSerial();
static bool initStorage(const BootConfig& config);
static bool initLogger(const BootConfig& config);
static bool initDisplay(const BootConfig& config);
static bool initTouch(const BootConfig& config);
static bool initLVGL();
static bool initUI(const BootConfig& config);

/* LVGL callbacks */
static void lvglFlushCb(lv_disp_drv_t* disp, const lv_area_t* area, lv_color_t* color_p);
static void lvglTouchCb(lv_indev_drv_t* drv, lv_indev_data_t* data);

/*===========================================================================*/
/*                            PUBLIC API                                      */
/*===========================================================================*/

BootConfig getDefaultBootConfig() {
    BootConfig config;
    config.enableSD = Features::ENABLE_SD_CARD;
    config.enableLogging = Features::ENABLE_LOGGING;
    config.enableTouch = Features::ENABLE_TOUCH;
    config.startDemo = true;
    config.displayBrightness = 100;
    return config;
}

BootResult boot(const BootConfig& config) {
    BootResult result = { true, BootStage::START, nullptr };
    
    if (s_booted) {
        result.success = true;
        result.errorMessage = "Already booted";
        return result;
    }
    
    Serial.println("\n========================================");
    Serial.println("       " SYSTEM_NAME);
    Serial.println("       Version: " SYSTEM_VERSION);
    Serial.println("========================================\n");
    
    /* Stage 1: Serial */
    s_currentStage = BootStage::SERIAL_INIT;
    if (!initSerial()) {
        result.success = false;
        result.failedStage = BootStage::SERIAL_INIT;
        result.errorMessage = "Serial init failed";
        return result;
    }
    
    /* Stage 2: Storage */
    s_currentStage = BootStage::STORAGE_INIT;
    if (config.enableSD && !initStorage(config)) {
        Serial.println("[BOOT] WARNING: Storage init failed, continuing without SD");
        /* Not fatal - continue without storage */
    }
    
    /* Stage 3: Logger */
    s_currentStage = BootStage::LOGGER_INIT;
    if (config.enableLogging && !initLogger(config)) {
        Serial.println("[BOOT] WARNING: Logger init failed, using Serial only");
        /* Not fatal */
    }
    
    /* Stage 4: Display */
    s_currentStage = BootStage::DISPLAY_INIT;
    if (!initDisplay(config)) {
        result.success = false;
        result.failedStage = BootStage::DISPLAY_INIT;
        result.errorMessage = "Display init failed";
        return result;
    }
    
    /* Stage 5: Touch */
    s_currentStage = BootStage::TOUCH_INIT;
    if (config.enableTouch && !initTouch(config)) {
        Serial.println("[BOOT] WARNING: Touch init failed, UI will be display-only");
        /* Not fatal */
    }
    
    /* Stage 6: LVGL */
    s_currentStage = BootStage::LVGL_INIT;
    if (!initLVGL()) {
        result.success = false;
        result.failedStage = BootStage::LVGL_INIT;
        result.errorMessage = "LVGL init failed";
        return result;
    }
    
    /* Stage 7: UI */
    s_currentStage = BootStage::UI_INIT;
    if (!initUI(config)) {
        result.success = false;
        result.failedStage = BootStage::UI_INIT;
        result.errorMessage = "UI init failed";
        return result;
    }
    
    /* Complete */
    s_currentStage = BootStage::COMPLETE;
    s_booted = true;
    
    Serial.println("\n[BOOT] ===== BOOT COMPLETE =====\n");
    
    if (Features::ENABLE_LOGGING) {
        log_info("BOOT", "System boot completed successfully");
    }
    
    return result;
}

BootResult boot() {
    return boot(getDefaultBootConfig());
}

BootStage getCurrentBootStage() {
    return s_currentStage;
}

const char* getBootStageName(BootStage stage) {
    switch (stage) {
        case BootStage::START:          return "Starting";
        case BootStage::SERIAL_INIT:    return "Serial";
        case BootStage::STORAGE_INIT:   return "Storage";
        case BootStage::LOGGER_INIT:    return "Logger";
        case BootStage::DISPLAY_INIT:   return "Display";
        case BootStage::TOUCH_INIT:     return "Touch";
        case BootStage::LVGL_INIT:      return "LVGL";
        case BootStage::UI_INIT:        return "UI";
        case BootStage::TASKS_INIT:     return "Tasks";
        case BootStage::COMPLETE:       return "Complete";
        case BootStage::FAILED:         return "FAILED";
        default:                        return "Unknown";
    }
}

void emergencyShutdown(const char* reason) {
    Serial.printf("\n!!! EMERGENCY SHUTDOWN: %s !!!\n", reason);
    
    if (s_display) {
        s_display->setBrightness(0);
    }
    
    /* Log if possible */
    if (Features::ENABLE_LOGGING) {
        log_error("BOOT", "Emergency shutdown: %s", reason);
    }
    
    /* Halt */
    while (true) {
        delay(1000);
    }
}

/*===========================================================================*/
/*                         INIT IMPLEMENTATIONS                               */
/*===========================================================================*/

static bool initSerial() {
    Serial.begin(115200);
    delay(100);  /* Let serial settle */
    Serial.println("[BOOT] Serial initialized");
    return true;
}

static bool initStorage(const BootConfig& config) {
    (void)config;
    
    if (sd_card_init()) {
        Serial.println("[BOOT] SD Card initialized");
        return true;
    }
    return false;
}

static bool initLogger(const BootConfig& config) {
    (void)config;
    
    logger_init();
    Serial.println("[BOOT] Logger initialized");
    return true;
}

static bool initDisplay(const BootConfig& config) {
    s_display = new HAL::NV3041ADisplay();
    
    if (!s_display || !s_display->init()) {
        Serial.println("[BOOT] Display init FAILED!");
        return false;
    }
    
    s_display->setBrightness(config.displayBrightness);
    Serial.printf("[BOOT] Display: %dx%d @ %d%% brightness\n",
        s_display->getWidth(), s_display->getHeight(), config.displayBrightness);
    
    return true;
}

static bool initTouch(const BootConfig& config) {
    (void)config;
    
    if (!s_display) {
        return false;
    }
    
    s_touch = new HAL::GT911Touch();
    
    if (!s_touch || !s_touch->init(
            s_display->getWidth(),
            s_display->getHeight(),
            s_display->getRotation())) {
        Serial.println("[BOOT] Touch init FAILED!");
        return false;
    }
    
    Serial.println("[BOOT] Touch initialized");
    return true;
}

static bool initLVGL() {
    if (!s_display) {
        return false;
    }
    
    uint32_t width = s_display->getWidth();
    uint32_t height = s_display->getHeight();
    uint32_t bufSize = width * LVGLConfig::BUFFER_LINES;
    
    /* Allocate draw buffer - prefer internal RAM */
    if (LVGLConfig::PREFER_INTERNAL_RAM) {
        s_dispDrawBuf = (lv_color_t*)heap_caps_malloc(
            sizeof(lv_color_t) * bufSize,
            MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT
        );
    }
    
    /* Fallback to PSRAM if needed */
    if (!s_dispDrawBuf && LVGLConfig::FALLBACK_TO_PSRAM) {
        s_dispDrawBuf = (lv_color_t*)heap_caps_malloc(
            sizeof(lv_color_t) * bufSize,
            MALLOC_CAP_8BIT
        );
    }
    
    if (!s_dispDrawBuf) {
        Serial.println("[BOOT] LVGL buffer allocation FAILED!");
        return false;
    }
    
    /* Initialize LVGL */
    lv_init();
    
    /* Setup display driver */
    lv_disp_draw_buf_init(&s_drawBuf, s_dispDrawBuf, nullptr, bufSize);
    
    lv_disp_drv_init(&s_dispDrv);
    s_dispDrv.hor_res = width;
    s_dispDrv.ver_res = height;
    s_dispDrv.flush_cb = lvglFlushCb;
    s_dispDrv.draw_buf = &s_drawBuf;
    lv_disp_drv_register(&s_dispDrv);
    
    /* Setup input driver if touch available */
    if (s_touch && s_touch->isInitialized()) {
        lv_indev_drv_init(&s_indevDrv);
        s_indevDrv.type = LV_INDEV_TYPE_POINTER;
        s_indevDrv.read_cb = lvglTouchCb;
        lv_indev_drv_register(&s_indevDrv);
    }
    
    Serial.printf("[BOOT] LVGL initialized (buffer: %lu bytes)\n",
        (unsigned long)(sizeof(lv_color_t) * bufSize));
    
    return true;
}

static bool initUI(const BootConfig& config) {
    if (config.startDemo) {
        lv_demo_widgets();
        Serial.println("[BOOT] Demo widgets started");
    }
    return true;
}

/*===========================================================================*/
/*                           LVGL CALLBACKS                                   */
/*===========================================================================*/

static void lvglFlushCb(lv_disp_drv_t* disp, const lv_area_t* area, lv_color_t* color_p) {
    if (s_display) {
        s_display->flush(disp, area, color_p);
    } else {
        lv_disp_flush_ready(disp);
    }
}

static void lvglTouchCb(lv_indev_drv_t* drv, lv_indev_data_t* data) {
    if (s_touch) {
        s_touch->lvglRead(drv, data);
    } else {
        data->state = LV_INDEV_STATE_REL;
    }
}

/*===========================================================================*/
/*                           PUBLIC ACCESSORS                                 */
/*===========================================================================*/

/* These functions provide access to HAL instances for legacy code */

} // namespace Core

/*===========================================================================*/
/*                        GLOBAL ACCESSOR FUNCTIONS                           */
/*===========================================================================*/

/* For backward compatibility with existing code */

extern "C" {

HAL::NV3041ADisplay* getDisplay() {
    return Core::s_display;
}

HAL::GT911Touch* getTouch() {
    return Core::s_touch;
}

void lvgl_handler() {
    lv_timer_handler();
    
    /* Flush canvas frame */
    if (Core::s_display) {
        Core::s_display->flushFrame();
    }
}

} // extern "C"
