/**
 * @file core.hpp
 * @brief ESP32 Web Framework - Core Module
 * 
 * Main server core responsible for:
 * - FreeRTOS task management
 * - Server lifecycle
 * - Module registration
 * - Watchdog functionality
 * 
 * @author ESP32 Web Framework
 * @version 1.0.0
 */

#ifndef ESP_WEB_FRAMEWORK_CORE_HPP
#define ESP_WEB_FRAMEWORK_CORE_HPP

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <functional>
#include <vector>
#include <memory>

#include "settings.h"
#include "http.hpp"
#include "router.hpp"
#include "middleware.hpp"
#include "logger.hpp"

namespace espweb {

/**
 * @brief Server state enumeration
 */
enum class ServerState {
    STOPPED,
    STARTING,
    RUNNING,
    STOPPING,
    ERROR
};

/**
 * @brief Web module interface for plugin system
 */
class WebModule {
public:
    virtual ~WebModule() = default;
    
    /**
     * @brief Register routes for this module
     * @param router Reference to the main router
     */
    virtual void registerRoutes(Router& router) = 0;
    
    /**
     * @brief Initialize the module
     * @return true if initialization successful
     */
    virtual bool init() { return true; }
    
    /**
     * @brief Get module name
     * @return Module name string
     */
    virtual const char* getName() const = 0;
};

/**
 * @brief Core server class - singleton pattern
 * 
 * Main entry point for the ESP32 Web Framework.
 * Manages the HTTP server, routing, and all registered modules.
 */
class WebServerCore {
public:
    /**
     * @brief Get singleton instance
     * @return Reference to WebServerCore instance
     */
    static WebServerCore& getInstance();
    
    // Delete copy constructor and assignment
    WebServerCore(const WebServerCore&) = delete;
    WebServerCore& operator=(const WebServerCore&) = delete;
    
    /**
     * @brief Initialize the server
     * @param port Port to listen on (default from settings)
     * @return true if initialization successful
     */
    bool init(uint16_t port = 0);
    
    /**
     * @brief Add middleware to the server
     * @param middleware Unique pointer to middleware
     */
    void use(std::unique_ptr<Middleware> middleware) {
        middleware_.add(std::move(middleware));
    }

    /**
     * @brief Start the server
     * Creates FreeRTOS task on specified core
     * @return true if server started successfully
     */
    bool start();
    
    /**
     * @brief Stop the server
     * @return true if server stopped successfully
     */
    bool stop();
    
    /**
     * @brief Restart the server without ESP reset
     * @return true if restart successful
     */
    bool restart();
    
    /**
     * @brief Get current server state
     * @return Current ServerState
     */
    ServerState getState() const { return state_; }
    
    /**
     * @brief Check if server is running
     * @return true if server is in RUNNING state
     */
    bool isRunning() const { return state_ == ServerState::RUNNING; }
    
    /**
     * @brief Get router reference
     * @return Reference to Router
     */
    Router& getRouter() { return router_; }
    
    /**
     * @brief Get middleware manager reference
     * @return Reference to MiddlewareManager
     */
    MiddlewareManager& getMiddleware() { return middleware_; }
    
    /**
     * @brief Register a web module
     * @param module Unique pointer to WebModule
     */
    void registerModule(std::unique_ptr<WebModule> module);
    
    /**
     * @brief Get server uptime in milliseconds
     * @return Uptime in ms
     */
    uint32_t getUptime() const;
    
    /**
     * @brief Get number of requests served
     * @return Request count
     */
    uint32_t getRequestCount() const { return requestCount_; }
    
    /**
     * @brief Get number of active connections
     * @return Active connection count
     */
    uint8_t getActiveConnections() const { return activeConnections_; }
    
    /**
     * @brief Feed the watchdog
     */
    void feedWatchdog();
    
private:
    WebServerCore();
    ~WebServerCore();
    
    /**
     * @brief Main server task function
     * @param parameter Pointer to WebServerCore instance
     */
    static void serverTask(void* parameter);
    
    /**
     * @brief Watchdog task function
     * @param parameter Pointer to WebServerCore instance
     */
    static void watchdogTask(void* parameter);
    
    /**
     * @brief Handle incoming client connection
     * @param client WiFiClient reference
     */
    void handleClient(WiFiClient& client);
    
    /**
     * @brief Parse HTTP request from client
     * @param client WiFiClient reference
     * @param request Request object to populate
     * @return true if parsing successful
     */
    bool parseRequest(WiFiClient& client, Request& request);
    
    /**
     * @brief Send response to client
     * @param client WiFiClient reference
     * @param response Response to send
     */
    void sendResponse(WiFiClient& client, Response& response);
    
    // Server components
    WiFiServer server_;
    uint16_t port_;
    Router router_;
    MiddlewareManager middleware_;
    std::vector<std::unique_ptr<WebModule>> modules_;
    
    // State
    ServerState state_;
    TaskHandle_t serverTaskHandle_;
    TaskHandle_t watchdogTaskHandle_;
    
    // Statistics
    uint32_t startTime_;
    volatile uint32_t requestCount_;
    volatile uint8_t activeConnections_;
    volatile uint32_t lastWatchdogFeed_;
    
    // Synchronization
    SemaphoreHandle_t mutex_;
};

/**
 * @brief Convenience function to get server instance
 * @return Reference to WebServerCore
 */
inline WebServerCore& WebServer() {
    return WebServerCore::getInstance();
}

} // namespace espweb

#endif // ESP_WEB_FRAMEWORK_CORE_HPP
