/**
 * @file wifi_config.h
 * @brief WiFi configuration for integrated system
 * 
 * Single source of truth for WiFi credentials
 */

#pragma once

namespace WifiConfig {
    
    // WiFi Credentials - UPDATE THESE
    constexpr const char* SSID = "paluszki_w_dipie";
    constexpr const char* PASSWORD = "39142914";
    
    // Web Server Configuration
    constexpr uint16_t HTTP_PORT = 80;
    constexpr uint16_t WEBSOCKET_PORT = 81;
    
    // Connection Timeouts (ms)
    constexpr uint32_t WIFI_CONNECT_TIMEOUT = 10000;  // 10 seconds
    constexpr uint32_t WIFI_RECONNECT_INTERVAL = 30000; // 30 seconds
    
    // Enable/Disable WiFi at boot
    constexpr bool ENABLE_WIFI_ON_BOOT = true;
    
    // WebSocket Update Intervals (ms)
    constexpr uint32_t SENSOR_UPDATE_INTERVAL = 1000;  // 1 second
    constexpr uint32_t STATUS_UPDATE_INTERVAL = 5000;  // 5 seconds
    
} // namespace WifiConfig
