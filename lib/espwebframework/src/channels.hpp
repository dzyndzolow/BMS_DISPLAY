/**
 * @file channels.hpp
 * @brief ESP32 Web Framework - WebSocket using ArduinoWebsockets library
 * 
 * Simple and reliable WebSocket implementation.
 * 
 * @author ESP32 Web Framework
 * @version 2.0.0
 */

#ifndef ESP_WEB_FRAMEWORK_CHANNELS_HPP
#define ESP_WEB_FRAMEWORK_CHANNELS_HPP

#include <Arduino.h>
#include <ArduinoWebsockets.h>
#include <vector>
#include <functional>
#include <ArduinoJson.h>
#include "settings.h"

using namespace websockets;

namespace espweb {

/**
 * @brief Simple WebSocket manager using ArduinoWebsockets
 */
class Channels {
public:
    using MessageHandler = std::function<void(const String&)>;
    using ClientHandler = std::function<void(void)>;

    static Channels& getInstance();
    
    Channels(const Channels&) = delete;
    Channels& operator=(const Channels&) = delete;
    
    /**
     * @brief Start WebSocket server
     */
    bool start(uint16_t port = 81);
    
    /**
     * @brief Stop WebSocket server
     */
    void stop();
    
    /**
     * @brief Process WebSocket events (call in loop or task)
     */
    void poll();
    
    /**
     * @brief Check if running
     */
    bool isRunning() const { return running_; }
    
    /**
     * @brief Broadcast message to all clients
     */
    void broadcast(const String& message);
    
    /**
     * @brief Broadcast JSON to all clients
     */
    void broadcast(const JsonDocument& doc);
    
    /**
     * @brief Get connected client count
     */
    size_t getClientCount() const { return clients_.size(); }
    
    /**
     * @brief Set message handler
     */
    void onMessage(MessageHandler handler) { messageHandler_ = handler; }
    
    /**
     * @brief Set connect handler
     */
    void onConnect(ClientHandler handler) { connectHandler_ = handler; }
    
    /**
     * @brief Set disconnect handler
     */
    void onDisconnect(ClientHandler handler) { disconnectHandler_ = handler; }

private:
    Channels() = default;
    ~Channels() = default;
    
    static void wsTask(void* parameter);
    
    WebsocketsServer server_;
    std::vector<WebsocketsClient> clients_;
    
    MessageHandler messageHandler_;
    ClientHandler connectHandler_;
    ClientHandler disconnectHandler_;
    
    TaskHandle_t taskHandle_ = nullptr;
    bool running_ = false;
    uint16_t port_ = 81;
};

// Convenience function
inline Channels& WS() {
    return Channels::getInstance();
}

} // namespace espweb

#endif // ESP_WEB_FRAMEWORK_CHANNELS_HPP
