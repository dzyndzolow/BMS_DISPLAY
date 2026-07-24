/**
 * @file channels.cpp
 * @brief ESP32 Web Framework - WebSocket Implementation using ArduinoWebsockets
 * 
 * @author ESP32 Web Framework
 * @version 2.0.0
 */

#include "channels.hpp"
#include "logger.hpp"

namespace espweb {

Channels& Channels::getInstance() {
    static Channels instance;
    return instance;
}

bool Channels::start(uint16_t port) {
    if (running_) {
        Serial.println("[WS] Already running");
        return true;
    }
    
    port_ = port;
    
    Serial.print("[WS] Starting WebSocket server on port ");
    Serial.println(port_);
    
    server_.listen(port_);
    
    if (!server_.available()) {
        Serial.println("[WS] ERROR: Server failed to start!");
        return false;
    }
    
    Serial.println("[WS] Server listening!");
    
    // Create background task
    running_ = true;
    
    BaseType_t result = xTaskCreatePinnedToCore(
        wsTask,
        "WebSocketTask",
        8192,
        this,
        2,
        &taskHandle_,
        1
    );
    
    if (result != pdPASS) {
        running_ = false;
        Serial.println("[WS] ERROR: Failed to create task");
        return false;
    }
    
    Serial.println("[WS] Task created successfully");
    return true;
}

void Channels::stop() {
    if (!running_) return;
    
    running_ = false;
    
    // Close all clients
    for (auto& client : clients_) {
        client.close();
    }
    clients_.clear();
    
    // WebsocketsServer doesn't have close() - just stop accepting
    
    if (taskHandle_) {
        vTaskDelete(taskHandle_);
        taskHandle_ = nullptr;
    }
    
    Serial.println("[WS] Server stopped");
}

void Channels::poll() {
    // Accept new clients
    if (server_.poll()) {
        WebsocketsClient client = server_.accept();
        if (client.available()) {
            Serial.println("[WS] New client connected!");
            
            // Set up message callback for this client
            client.onMessage([this](WebsocketsClient&, WebsocketsMessage msg) {
                String data = msg.data();
                Serial.print("[WS] Received: ");
                Serial.println(data);
                
                if (messageHandler_) {
                    messageHandler_(data);
                }
            });
            
            clients_.push_back(std::move(client));
            
            if (connectHandler_) {
                connectHandler_();
            }
            
            Serial.print("[WS] Total clients: ");
            Serial.println(clients_.size());
        }
    }
    
    // Poll existing clients and remove disconnected ones
    for (int i = clients_.size() - 1; i >= 0; i--) {
        if (clients_[i].available()) {
            clients_[i].poll();
        } else {
            Serial.println("[WS] Client disconnected");
            clients_.erase(clients_.begin() + i);
            
            if (disconnectHandler_) {
                disconnectHandler_();
            }
            
            Serial.print("[WS] Remaining clients: ");
            Serial.println(clients_.size());
        }
    }
}

void Channels::wsTask(void* parameter) {
    Channels* ws = static_cast<Channels*>(parameter);
    
    Serial.println("[WS] Task started");
    
    for (;;) {
        if (!ws->running_) {
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }
        
        ws->poll();
        
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void Channels::broadcast(const String& message) {
    if (clients_.empty()) return;
    
    int sent = 0;
    for (auto& client : clients_) {
        if (client.available()) {
            client.send(message);
            sent++;
        }
    }
    
    // Uncomment for debug:
    // Serial.print("[WS] Broadcast to ");
    // Serial.print(sent);
    // Serial.println(" clients");
}

void Channels::broadcast(const JsonDocument& doc) {
    String json;
    serializeJson(doc, json);
    broadcast(json);
}

} // namespace espweb
