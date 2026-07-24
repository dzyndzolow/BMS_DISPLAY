/**
 * @file settings.h
 * @brief ESP32 Web Framework - Global Settings Configuration
 * 
 * This file contains all configurable settings for the framework.
 * Inspired by Django's settings.py
 * 
 * @author ESP32 Web Framework
 * @version 1.0.0
 */

#ifndef ESP_WEB_FRAMEWORK_SETTINGS_H
#define ESP_WEB_FRAMEWORK_SETTINGS_H

#include <cstdint>

namespace espweb {
namespace settings {

//==============================================================================
// Server Configuration
//==============================================================================

/** @brief HTTP server port */
constexpr uint16_t SERVER_PORT = 80;

/** @brief WebSocket server port */
constexpr uint16_t WEBSOCKET_PORT = 81;

/** @brief FreeRTOS core for server task (0 or 1) */
constexpr uint8_t SERVER_CORE = 1;

/** @brief FreeRTOS task priority for server */
constexpr uint8_t SERVER_TASK_PRIORITY = 4;

/** @brief Server task stack size in bytes */
constexpr uint32_t SERVER_STACK_SIZE = 8192;

/** @brief Maximum concurrent client connections */
constexpr uint8_t MAX_CLIENTS = 8;

/** @brief Connection timeout in milliseconds */
constexpr uint32_t CONNECTION_TIMEOUT_MS = 5000;

/** @brief Keep-alive timeout in milliseconds */
constexpr uint32_t KEEP_ALIVE_TIMEOUT_MS = 15000;

//==============================================================================
// File System Configuration
//==============================================================================

/** @brief Root path for static files */
#define STATIC_DIR "/static"

/** @brief Static URL path */
#define STATIC_URL_PATH "/static"

/** @brief Root path for template files */
#define TEMPLATE_DIR "/templates"

/** @brief Templates directory (alias) */
#define TEMPLATES_DIR "/templates"

/** @brief SD card mount point */
#define SD_MOUNT_POINT "/sd"

/** @brief SPIFFS mount point */
#define SPIFFS_MOUNT_POINT "/spiffs"

/** @brief Use SD card */
constexpr bool USE_SD_CARD = false;

/** @brief SD card CS pin */
constexpr int SD_CS_PIN = 5;

/** @brief Use SPIFFS */
constexpr bool USE_SPIFFS = true;

/** @brief Maximum upload file size in bytes (10 MB) */
constexpr size_t MAX_UPLOAD_SIZE = 10 * 1024 * 1024;

/** @brief Upload buffer size in bytes */
constexpr size_t UPLOAD_BUFFER_SIZE = 4096;

//==============================================================================
// Template Engine Configuration
//==============================================================================

/** @brief Enable template caching in PSRAM */
constexpr bool TEMPLATE_CACHE_ENABLED = true;

/** @brief Maximum number of cached templates */
constexpr size_t TEMPLATE_CACHE_MAX_ITEMS = 50;

/** @brief Template cache TTL in seconds */
constexpr uint32_t TEMPLATE_CACHE_TTL_SECONDS = 3600;

//==============================================================================
// Cache Configuration
//==============================================================================

/** @brief Enable static file caching */
constexpr bool STATIC_CACHE_ENABLED = true;

/** @brief Maximum size of cached file in bytes */
constexpr size_t STATIC_CACHE_MAX_FILE_SIZE = 64 * 1024;

/** @brief Maximum total cache size in bytes (1 MB) */
constexpr size_t STATIC_CACHE_MAX_TOTAL_SIZE = 1 * 1024 * 1024;

/** @brief Cache TTL in seconds */
constexpr uint32_t CACHE_TTL_SECONDS = 3600;

/** @brief String cache size in bytes */
constexpr size_t STRING_CACHE_SIZE = 256 * 1024;

/** @brief Binary cache size in bytes */
constexpr size_t BINARY_CACHE_SIZE = 512 * 1024;

/** @brief Response cache size in bytes */
constexpr size_t RESPONSE_CACHE_SIZE = 256 * 1024;

/** @brief Response cache TTL in seconds */
constexpr uint32_t RESPONSE_CACHE_TTL = 300;

//==============================================================================
// Logging Configuration
//==============================================================================

/** @brief Debug mode - enables verbose logging */
constexpr bool DEBUG = true;

/** @brief Debug mode alias */
constexpr bool DEBUG_MODE = true;

/** @brief Enable logging to Serial */
constexpr bool LOG_TO_SERIAL = true;

/** @brief Enable logging to SD card */
constexpr bool LOG_TO_SD = false;

/** @brief Log file path on SD */
#define LOG_FILE_PATH "/sd/logs/server.log"

/** @brief Maximum log file size before rotation (1 MB) */
constexpr size_t LOG_MAX_FILE_SIZE = 1 * 1024 * 1024;

/** @brief Number of log files to keep */
constexpr uint8_t LOG_MAX_FILES = 5;

//==============================================================================
// WebSocket Configuration
//==============================================================================

/** @brief WebSocket task core */
constexpr uint8_t WEBSOCKET_CORE = 1;

/** @brief WebSocket task priority */
constexpr uint8_t WEBSOCKET_TASK_PRIORITY = 3;

/** @brief WebSocket task stack size */
constexpr uint32_t WEBSOCKET_STACK_SIZE = 4096;

/** @brief Maximum WebSocket clients */
constexpr uint8_t WEBSOCKET_MAX_CLIENTS = 16;

/** @brief WebSocket ping interval in milliseconds */
constexpr uint32_t WEBSOCKET_PING_INTERVAL_MS = 30000;

//==============================================================================
// Cron/Scheduler Configuration
//==============================================================================

/** @brief Cron task core */
constexpr uint8_t CRON_CORE = 1;

/** @brief Cron task priority */
constexpr uint8_t CRON_TASK_PRIORITY = 2;

/** @brief Cron task stack size */
constexpr uint32_t CRON_STACK_SIZE = 4096;

/** @brief Minimum cron interval in milliseconds */
constexpr uint32_t CRON_MIN_INTERVAL_MS = 100;

//==============================================================================
// ORM Configuration
//==============================================================================

/** @brief ORM data directory */
#define ORM_DATA_DIR "/sd/data"

/** @brief Enable ORM auto-save */
constexpr bool ORM_AUTO_SAVE = true;

/** @brief ORM auto-save interval in seconds */
constexpr uint32_t ORM_AUTO_SAVE_INTERVAL_SECONDS = 60;

//==============================================================================
// Security Configuration
//==============================================================================

/** @brief Enable CORS */
constexpr bool CORS_ENABLED = true;

/** @brief CORS allowed origins (use "*" for all) */
#define CORS_ALLOWED_ORIGINS "*"

/** @brief Enable rate limiting */
constexpr bool RATE_LIMIT_ENABLED = false;

/** @brief Rate limit requests per minute */
constexpr uint16_t RATE_LIMIT_REQUESTS_PER_MINUTE = 60;

//==============================================================================
// Diagnostics Configuration
//==============================================================================

/** @brief Enable diagnostics panel at /_diagnostics */
constexpr bool DIAGNOSTICS_ENABLED = true;

/** @brief Diagnostics panel path */
#define DIAGNOSTICS_PATH "/_diagnostics"

/** @brief Profiler enabled */
constexpr bool PROFILER_ENABLED = true;

//==============================================================================
// Watchdog Configuration
//==============================================================================

/** @brief Enable server watchdog */
constexpr bool WATCHDOG_ENABLED = true;

/** @brief Watchdog timeout in seconds */
constexpr uint32_t WATCHDOG_TIMEOUT_SECONDS = 30;

/** @brief Auto-restart server on hang */
constexpr bool WATCHDOG_AUTO_RESTART = true;

} // namespace settings
} // namespace espweb

#endif // ESP_WEB_FRAMEWORK_SETTINGS_H
