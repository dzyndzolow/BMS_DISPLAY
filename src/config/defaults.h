/**
 * @file defaults.h
 * @brief Centralized configuration - SINGLE SOURCE OF TRUTH
 * 
 * All hardware pins, display parameters, system constants, and feature flags
 * are defined here. NO hardcoded values should exist anywhere else in the codebase.
 * 
 * @version 1.0.0 (MASTER_PLAN Phase 0)
 */

#ifndef CONFIG_DEFAULTS_H
#define CONFIG_DEFAULTS_H

#include <cstdint>

/*===========================================================================*/
/*                              SYSTEM CONFIG                                 */
/*===========================================================================*/

#define SYSTEM_NAME             "ESP32-S3 LVGL System"
#define SYSTEM_VERSION          "1.0.0"
#define SYSTEM_BUILD_DATE       __DATE__ " " __TIME__

/*CPU Configuration*/
#define CPU_FREQ_MHZ            240
#define CORE_LVGL               0       /* Core for LVGL task */
#define CORE_SYSTEM             1       /* Core for system tasks */

/*===========================================================================*/
/*                            DISPLAY CONFIG                                  */
/*===========================================================================*/

/*Display Hardware - NV3041A via QSPI*/
namespace DisplayConfig {
    /*QSPI Bus Pins*/
    constexpr uint8_t PIN_CS    = 45;
    constexpr uint8_t PIN_SCK   = 47;
    constexpr uint8_t PIN_D0    = 21;
    constexpr uint8_t PIN_D1    = 48;
    constexpr uint8_t PIN_D2    = 40;
    constexpr uint8_t PIN_D3    = 39;
    
    /*Backlight*/
    constexpr uint8_t PIN_BL    = 1;
    constexpr uint32_t BL_PWM_FREQ = 20000;
    constexpr uint8_t BL_PWM_RES = 8;
    constexpr uint8_t BL_DEFAULT_BRIGHTNESS = 255;
    
    /*Display Parameters*/
    constexpr uint16_t WIDTH    = 480;
    constexpr uint16_t HEIGHT   = 272;
    constexpr uint8_t ROTATION  = 0;
    constexpr bool IPS_PANEL    = true;
    
    /*Colors (16-bit RGB565)*/
    constexpr uint16_t COLOR_BLACK = 0x0000;
    constexpr uint16_t COLOR_WHITE = 0xFFFF;
}

/*===========================================================================*/
/*                             TOUCH CONFIG                                   */
/*===========================================================================*/

/*Touch Hardware - GT911 via I2C*/
namespace TouchConfig {
    /*I2C Pins*/
    constexpr uint8_t PIN_SDA   = 8;
    constexpr uint8_t PIN_SCL   = 4;
    constexpr uint8_t PIN_INT   = 3;
    constexpr uint8_t PIN_RES   = 38;
    
    /*GT911 I2C Address*/
    constexpr uint8_t I2C_ADDR_1 = 0x5D;
    constexpr uint8_t I2C_ADDR_2 = 0x14;
    
    /*Touch Panel Parameters (matches display)*/
    constexpr uint16_t MAX_X    = DisplayConfig::WIDTH - 1;
    constexpr uint16_t MAX_Y    = DisplayConfig::HEIGHT - 1;
    
    /*Touch Timing*/
    constexpr uint16_t RESET_DELAY_MS = 200;
}

/*===========================================================================*/
/*                            SD CARD CONFIG                                  */
/*===========================================================================*/

/*SD Card Hardware - HSPI Bus*/
namespace SDConfig {
    /*HSPI Pins*/
    constexpr uint8_t PIN_CS    = 10;
    constexpr uint8_t PIN_MOSI  = 11;
    constexpr uint8_t PIN_SCK   = 12;
    constexpr uint8_t PIN_MISO  = 13;
    
    /*SPI Speed*/
    constexpr uint32_t SPI_FREQ_HZ = 25000000;  /* 25MHz */
    
    /*File System*/
    constexpr const char* MOUNT_POINT = "/sd";
    constexpr uint16_t MAX_FILES = 10;
}

/*===========================================================================*/
/*                            LOGGER CONFIG                                   */
/*===========================================================================*/

namespace LoggerConfig {
    /*Log Files*/
    constexpr const char* LOG_DIR       = "/logs";
    constexpr const char* LOG_FILENAME  = "system.log";
    constexpr uint32_t MAX_FILE_SIZE    = 1024 * 100;  /* 100KB */
    constexpr uint8_t MAX_LOG_FILES     = 5;           /* Rotation count */
    
    /*Log Levels*/
    enum class Level : uint8_t {
        DEBUG   = 0,
        INFO    = 1,
        WARNING = 2,
        ERROR   = 3,
        FATAL   = 4
    };
    
    /*Default Level*/
    constexpr Level DEFAULT_LEVEL = Level::INFO;
    
    /*Timestamps*/
    constexpr bool ENABLE_TIMESTAMPS = true;
}

/*===========================================================================*/
/*                            FREERTOS CONFIG                                 */
/*===========================================================================*/

namespace TaskConfig {
    /*LVGL Task*/
    constexpr uint32_t LVGL_STACK_SIZE  = 8192;
    constexpr uint8_t LVGL_PRIORITY     = 2;
    constexpr uint8_t LVGL_CORE         = CORE_LVGL;
    constexpr uint16_t LVGL_TICK_MS     = 5;
    
    /*System Task*/
    constexpr uint32_t SYSTEM_STACK_SIZE = 4096;
    constexpr uint8_t SYSTEM_PRIORITY    = 1;
    constexpr uint8_t SYSTEM_CORE        = CORE_SYSTEM;
    
    /*Watchdog*/
    constexpr uint32_t WDT_TIMEOUT_MS   = 5000;
}

/*===========================================================================*/
/*                              LVGL CONFIG                                   */
/*===========================================================================*/

namespace LVGLConfig {
    /*Draw Buffer*/
    constexpr uint8_t BUFFER_LINES      = 40;   /* Lines per buffer (non-direct mode) */
    constexpr bool USE_DOUBLE_BUFFER    = false;
    
    /*Memory*/
    constexpr bool PREFER_INTERNAL_RAM  = true;
    constexpr bool FALLBACK_TO_PSRAM    = true;
    
    /*Features*/
    constexpr bool ENABLE_ANTIALIASING  = true;
    constexpr bool ENABLE_SHADOWS       = true;
}

/*===========================================================================*/
/*                           FEATURE FLAGS                                    */
/*===========================================================================*/

namespace Features {
    /*Enable/Disable major features at compile time*/
    constexpr bool ENABLE_SD_CARD       = true;
    constexpr bool ENABLE_LOGGING       = true;
    constexpr bool ENABLE_TOUCH         = true;
    constexpr bool ENABLE_WIFI          = false;    /* Future */
    constexpr bool ENABLE_OTA           = false;    /* Future */
    constexpr bool ENABLE_PERF_MONITOR  = true;
    
    /*Debug Features*/
    constexpr bool DEBUG_TOUCH_COORDS   = false;
    constexpr bool DEBUG_MEMORY_USAGE   = false;
    constexpr bool DEBUG_FPS_COUNTER    = true;
}

/*===========================================================================*/
/*                          BACKWARD COMPATIBILITY                            */
/*===========================================================================*/

/*Legacy defines for gradual migration - DEPRECATED, use namespaced versions*/
#define TOUCH_SCL       TouchConfig::PIN_SCL
#define TOUCH_SDA       TouchConfig::PIN_SDA
#define TOUCH_RES       TouchConfig::PIN_RES
#define TOUCH_INT       TouchConfig::PIN_INT
#define SCREEN_WIDTH    DisplayConfig::WIDTH
#define SCREEN_HEIGHT   DisplayConfig::HEIGHT
#define GFX_BL          DisplayConfig::PIN_BL

#endif /* CONFIG_DEFAULTS_H */
