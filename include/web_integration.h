/**
 * @file web_integration.h
 * @brief Integration layer between LVGL UI and Web Framework
 * 
 * Provides:
 * - WebServer initialization for HTTP/WebSocket
 * - Event publishing from LVGL to Web clients
 * - Event subscription for Web API commands to UI
 */

#pragma once

#include <Arduino.h>
#include <espwebframework.h>
#include <memory>

namespace webint {

/**
 * @class WebIntegration
 * @brief Singleton managing LVGL <-> Web bidirectional communication
 */
class WebIntegration {
public:
    static WebIntegration& getInstance();
    
    /**
     * @brief Initialize web server with WiFi
     * @param ssid WiFi SSID
     * @param password WiFi password
     * @param port HTTP server port (default 80)
     * @param wwwRoot Root directory for web files on SD (default "/www")
     */
    bool init(const char* ssid, const char* password, uint16_t port = 80, const char* wwwRoot = "/www");
    
    /**
     * @brief Start web server (call after WiFi connected)
     */
    void start();
    
    /**
     * @brief Publish display brightness change to web clients
     * @param brightness 0-100
     */
    void publishBrightnessChange(uint8_t brightness);
    
    /**
     * @brief Publish screen change event
     * @param screenName Name of current screen
     */
    void publishScreenChange(const char* screenName);
    
    /**
     * @brief Publish sensor data to clients
     * @param sensorName Sensor identifier
     * @param value Sensor reading
     * @param unit Unit of measurement
     */
    void publishSensorData(const char* sensorName, float value, const char* unit);
    
    /**
     * @brief Broadcast message to all WebSocket clients
     * @param channel Channel name (e.g., "/ws/ui-events")
     * @param message JSON message
     */
    void broadcastMessage(const char* channel, const char* message);
    
    /**
     * @brief Check if WiFi is connected
     */
    bool isWiFiConnected() const;
    
    /**
     * @brief Get device uptime in seconds
     */
    uint32_t getUptime() const;
    
private:
    WebIntegration() = default;
    WebIntegration(const WebIntegration&) = delete;
    WebIntegration& operator=(const WebIntegration&) = delete;
    
    void setupStaticFiles();
    
    bool wifiConnected = false;
    uint32_t bootTime = 0;
    String wwwRootPath = "/www";
};

} // namespace webint
