/**
 * @file web_integration.cpp
 * @brief Web Framework integration implementation
 */

#include <web_integration.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <SD.h>

using namespace espweb;

namespace webint {

WebIntegration& WebIntegration::getInstance() {
    static WebIntegration instance;
    return instance;
}

bool WebIntegration::init(const char* ssid, const char* password, uint16_t port, const char* wwwRoot) {
    Serial.println("[WebInt] Initializing WiFi...");
    
    wwwRootPath = wwwRoot;
    Serial.printf("[WebInt] Web root directory: %s\n", wwwRootPath.c_str());
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    
    // Wait for WiFi connection (max 10 seconds)
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        wifiConnected = true;
        Serial.println("\n[WebInt] WiFi connected!");
        Serial.print("[WebInt] IP Address: ");
        Serial.println(WiFi.localIP());
        
        bootTime = millis() / 1000;
        
        // Initialize ESP Web Framework
        Serial.println("[WebInt] Initializing ESP Web Framework...");
        WebServer().init(port);
        
        return true;
    }
    
    Serial.println("\n[WebInt] WiFi connection failed");
    return false;
}

void WebIntegration::start() {
    if (!wifiConnected) {
        Serial.println("[WebInt] Cannot start - WiFi not connected");
        return;
    }
    
    Serial.println("[WebInt] Registering routes...");
    
    // Static files from SD card
    setupStaticFiles();
    
    // Home endpoint - Serve index.html from SD card
    Routes().get("/", [this](Request& req) {
        String indexPath = wwwRootPath + "/index.html";
        
        if (!SD.exists(indexPath)) {
            // Fallback to JSON status if no index.html
            JsonDocument doc;
            doc["status"] = "online";
            doc["device"] = "ESP32-S3 LVGL System";
            doc["message"] = "No index.html found on SD card";
            doc["path"] = indexPath;
            doc["uptime"] = millis() / 1000;
            doc["freeHeap"] = ESP.getFreeHeap();
            doc["freePSRAM"] = ESP.getFreePsram();
            return Response::json(doc);
        }
        
        File file = SD.open(indexPath, FILE_READ);
        if (!file) {
            JsonDocument doc;
            doc["error"] = "Cannot open index.html";
            return Response::json(doc);
        }
        
        String content;
        content.reserve(file.size());
        while (file.available()) {
            content += (char)file.read();
        }
        file.close();
        
        Response response;
        response.body = content;
        response.headers["Content-Type"] = "text/html";
        return response;
    });
    
    // API: Get system status
    Routes().get("/api/status", [](Request& req) {
        JsonDocument doc;
        doc["uptime"] = millis();
        doc["freeHeap"] = ESP.getFreeHeap();
        doc["freePsram"] = ESP.getFreePsram();
        doc["temperature"] = (int)temperatureRead();
        doc["rssi"] = WiFi.RSSI();
        doc["ssid"] = WiFi.SSID();
        doc["ip"] = WiFi.localIP().toString();
        return Response::json(doc);
    });
    
    // API: Control brightness
    Routes().post("/api/brightness", [](Request& req) {
        JsonDocument doc;
        
        if (req.body.isEmpty()) {
            doc["error"] = "Body required";
            return Response::json(doc);
        }
        
        JsonDocument reqDoc;
        deserializeJson(reqDoc, req.body);
        
        if (!reqDoc.containsKey("brightness")) {
            doc["error"] = "Missing 'brightness' parameter";
            return Response::json(doc);
        }
        
        uint8_t brightness = reqDoc["brightness"].as<uint8_t>();
        
        // TODO: Call display HAL to set brightness
        // DisplayManager::getInstance().setBrightness(brightness);
        
        doc["brightness"] = brightness;
        doc["status"] = "updated";
        return Response::json(doc);
    });
    
    // API: Screen information
    Routes().get("/api/screen", [](Request& req) {
        JsonDocument doc;
        // TODO: Get current screen info from ScreenManager
        doc["currentScreen"] = "home";
        doc["availableScreens"] = JsonArray();
        return Response::json(doc);
    });
    
    // Start the actual HTTP server!
    Serial.println("[WebInt] Starting HTTP server...");
    if (WebServer().start()) {
        Serial.println("[WebInt] Web server started successfully!");
        Serial.print("[WebInt] Open http://");
        Serial.print(WiFi.localIP());
        Serial.println("/ in browser");
    } else {
        Serial.println("[WebInt] ERROR: Failed to start web server!");
    }
}

void WebIntegration::publishBrightnessChange(uint8_t brightness) {
    JsonDocument doc;
    doc["event"] = "brightness_changed";
    doc["value"] = brightness;
    doc["timestamp"] = millis();
    
    String payload;
    serializeJson(doc, payload);
    broadcastMessage("/ws/ui-events", payload.c_str());
}

void WebIntegration::publishScreenChange(const char* screenName) {
    JsonDocument doc;
    doc["event"] = "screen_changed";
    doc["screen"] = screenName;
    doc["timestamp"] = millis();
    
    String payload;
    serializeJson(doc, payload);
    broadcastMessage("/ws/ui-events", payload.c_str());
}

void WebIntegration::publishSensorData(const char* sensorName, float value, const char* unit) {
    JsonDocument doc;
    doc["event"] = "sensor_data";
    doc["sensor"] = sensorName;
    doc["value"] = value;
    doc["unit"] = unit;
    doc["timestamp"] = millis();
    
    String payload;
    serializeJson(doc, payload);
    broadcastMessage("/ws/sensors", payload.c_str());
}

void WebIntegration::broadcastMessage(const char* channel, const char* message) {
    if (!wifiConnected) return;
    
    // TODO: Implement WebSocket broadcast through espwebframework
    // Channels().broadcast(channel, message);
}

bool WebIntegration::isWiFiConnected() const {
    return wifiConnected && WiFi.status() == WL_CONNECTED;
}

uint32_t WebIntegration::getUptime() const {
    return (millis() / 1000) - bootTime;
}

void WebIntegration::setupStaticFiles() {
    Serial.printf("[WebInt] Setting up static file serving from %s\n", wwwRootPath.c_str());
    
    // Serve static files from SD card
    // Pattern: /static/* -> SD:/www/*
    Routes().get("/static/*", [this](Request& req) {
        String path = req.path;
        path.replace("/static", wwwRootPath);
        
        Serial.printf("[WebInt] Serving file: %s\n", path.c_str());
        
        if (!SD.exists(path)) {
            JsonDocument doc;
            doc["error"] = "File not found";
            doc["path"] = path;
            return Response::json(doc);
        }
        
        File file = SD.open(path, FILE_READ);
        if (!file) {
            JsonDocument doc;
            doc["error"] = "Cannot open file";
            return Response::json(doc);
        }
        
        // Determine content type from extension
        String contentType = "text/plain";
        if (path.endsWith(".html")) contentType = "text/html";
        else if (path.endsWith(".css")) contentType = "text/css";
        else if (path.endsWith(".js")) contentType = "application/javascript";
        else if (path.endsWith(".json")) contentType = "application/json";
        else if (path.endsWith(".png")) contentType = "image/png";
        else if (path.endsWith(".jpg") || path.endsWith(".jpeg")) contentType = "image/jpeg";
        else if (path.endsWith(".gif")) contentType = "image/gif";
        else if (path.endsWith(".svg")) contentType = "image/svg+xml";
        else if (path.endsWith(".ico")) contentType = "image/x-icon";
        
        // Read file content
        size_t fileSize = file.size();
        String content;
        content.reserve(fileSize);
        while (file.available()) {
            content += (char)file.read();
        }
        file.close();
        
        // Create response with proper content type
        Response response;
        response.body = content;
        response.headers["Content-Type"] = contentType;
        return response;
    });
    
    // Serve all files from www root (css/*, js/*, pages/*)
    Routes().get("/*", [this](Request& req) {
        String path = req.path;
        
        // Skip if already handled (/, /api/*, /static/*, /web)
        if (path == "/" || path.startsWith("/api/") || path.startsWith("/static/") || path == "/web") {
            return Response(); // Let other routes handle
        }
        
        String filePath = wwwRootPath + path;
        Serial.printf("[WebInt] Serving: %s\n", filePath.c_str());
        
        if (!SD.exists(filePath)) {
            JsonDocument doc;
            doc["error"] = "Not found";
            doc["path"] = path;
            Response response = Response::json(doc);
            response.status = espweb::HttpStatus::NOT_FOUND;
            return response;
        }
        
        File file = SD.open(filePath, FILE_READ);
        if (!file) {
            JsonDocument doc;
            doc["error"] = "Cannot open file";
            Response response = Response::json(doc);
            response.status = espweb::HttpStatus::INTERNAL_SERVER_ERROR;
            return response;
        }
        
        // Determine content type
        String contentType = "text/plain";
        if (path.endsWith(".html")) contentType = "text/html";
        else if (path.endsWith(".css")) contentType = "text/css";
        else if (path.endsWith(".js")) contentType = "application/javascript";
        else if (path.endsWith(".json")) contentType = "application/json";
        else if (path.endsWith(".png")) contentType = "image/png";
        else if (path.endsWith(".jpg") || path.endsWith(".jpeg")) contentType = "image/jpeg";
        
        String content;
        content.reserve(file.size());
        while (file.available()) {
            content += (char)file.read();
        }
        file.close();
        
        Response response;
        response.body = content;
        response.headers["Content-Type"] = contentType;
        return response;
    });
    
    // Serve index.html from root if exists
    Routes().get("/web", [this](Request& req) {
        String indexPath = wwwRootPath + "/index.html";
        
        if (!SD.exists(indexPath)) {
            JsonDocument doc;
            doc["message"] = "Web interface not found";
            doc["info"] = "Upload files to SD:" + wwwRootPath;
            return Response::json(doc);
        }
        
        File file = SD.open(indexPath, FILE_READ);
        String content;
        while (file.available()) {
            content += (char)file.read();
        }
        file.close();
        
        Response response;
        response.body = content;
        response.headers["Content-Type"] = "text/html";
        return response;
    });
    
    Serial.println("[WebInt] Static file routes registered");
}

} // namespace webint
