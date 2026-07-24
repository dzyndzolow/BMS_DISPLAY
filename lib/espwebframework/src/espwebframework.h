/**
 * @file espwebframework.h
 * @brief ESP32 Web Framework - Main Header
 * 
 * Django-inspired C++ web framework for ESP32-S3 with PSRAM.
 * 
 * Usage:
 *   #include <espwebframework.h>
 *   using namespace espweb;
 * 
 * @author ESP32 Web Framework
 * @version 1.0.0
 */

#ifndef ESPWEBFRAMEWORK_H
#define ESPWEBFRAMEWORK_H

// Framework version
#define ESPWEB_VERSION_MAJOR 1
#define ESPWEB_VERSION_MINOR 0
#define ESPWEB_VERSION_PATCH 0
#define ESPWEB_VERSION "1.0.0"

// Core components
#include "settings.h"
#include "http.hpp"
#include "router.hpp"
#include "core.hpp"

// Views and templates
#include "views.hpp"
#include "template.hpp"

// Middleware
#include "middleware.hpp"

// Static files and file I/O
#include "static.hpp"
#include "fileio.hpp"

// Real-time features
#include "channels.hpp"

// Data persistence
#include "orm.hpp"

// Scheduled tasks
#include "cron.hpp"

// Utilities
#include "logger.hpp"
#include "diagnostics.hpp"
#include "cache.hpp"

namespace espweb {

/**
 * @brief Initialize the entire framework
 * 
 * This function initializes all framework components in the correct order.
 * Call this in your setup() function before using any framework features.
 * 
 * @return true if initialization successful
 */
inline bool initFramework() {
    // Initialize logger first
    Logger::getInstance().init();
    LOG_INFO("Framework", "ESP Web Framework " ESPWEB_VERSION " starting...");
    
    // Initialize cache manager
    Caches().init();
    
    // Initialize template engine
    Templates().init(TEMPLATES_DIR);
    
    // Initialize static file handler
    StaticFiles().init(STATIC_DIR, STATIC_URL_PATH);
    
    // Initialize file I/O
    Files().init(settings::USE_SD_CARD, settings::USE_SPIFFS);
    
    // Initialize ORM
    DB().init(ORM_DATA_DIR);
    
    // WebSocket will be initialized when start() is called
    // Channels::getInstance() - no init needed
    
    // Initialize diagnostics
    Diag().init();
    
    // Initialize scheduler (but don't start yet)
    Scheduler::getInstance();
    
    LOG_INFO("Framework", "Framework initialization complete");
    return true;
}

/**
 * @brief Start the web server
 * 
 * This starts the HTTP server on the configured port.
 * WiFi must be connected before calling this.
 * 
 * @return true if server started successfully
 */
inline bool startServer() {
    return WebServer().start();
}

/**
 * @brief Stop the web server
 */
inline void stopServer() {
    WebServer().stop();
}

/**
 * @brief Quick setup for simple projects
 * 
 * Combines framework initialization and server start.
 * 
 * @param ssid WiFi SSID
 * @param password WiFi password
 * @param port Server port (default from settings)
 * @return true if setup successful
 */
inline bool quickSetup(const char* ssid, const char* password, 
                       uint16_t port = settings::SERVER_PORT) {
    // Connect to WiFi
    LOG_INFO("Setup", "Connecting to WiFi...");
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        attempts++;
    }
    
    if (WiFi.status() != WL_CONNECTED) {
        LOG_ERROR("Setup", "WiFi connection failed!");
        return false;
    }
    
    LOG_INFO("Setup", "WiFi connected: " + WiFi.localIP().toString());
    
    // Initialize framework
    if (!initFramework()) {
        return false;
    }
    
    // Configure server
    WebServer().init();
    
    // Register diagnostics routes if enabled
    if (settings::DEBUG_MODE) {
        Diag().registerRoutes(Routes());
    }
    
    // Start server
    return startServer();
}

/**
 * @brief Helper macro for defining routes
 */
#define ROUTE(method, path, handler) \
    Routes().method(path, handler)

#define GET(path, handler) ROUTE(get, path, handler)
#define POST(path, handler) ROUTE(post, path, handler)
#define PUT(path, handler) ROUTE(put, path, handler)
#define DELETE_ROUTE(path, handler) ROUTE(del, path, handler)

/**
 * @brief Helper macro for defining class-based views
 */
#define VIEW(path, ViewClass) \
    Routes().get(path, [](Request& req) { \
        ViewClass view; \
        return view.dispatch(req); \
    })

/**
 * @brief Helper macro for scheduled tasks
 */
#define SCHEDULE(interval, callback) \
    Scheduler::getInstance().schedule(#callback, interval, callback)

/**
 * @brief Helper macro for WebSocket handlers
 */
#define WEBSOCKET(path, handler) \
    Channels().registerHandler(path, handler)

} // namespace espweb

#endif // ESPWEBFRAMEWORK_H
