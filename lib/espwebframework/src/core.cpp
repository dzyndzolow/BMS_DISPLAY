/**
 * @file core.cpp
 * @brief ESP32 Web Framework - Core Implementation
 * 
 * @author ESP32 Web Framework
 * @version 1.0.0
 */

#include "core.hpp"
#include "static.hpp"
#include "template.hpp"
#include "diagnostics.hpp"

namespace espweb {

// Singleton instance
WebServerCore& WebServerCore::getInstance() {
    static WebServerCore instance;
    return instance;
}

WebServerCore::WebServerCore() 
    : server_(settings::SERVER_PORT),
      port_(settings::SERVER_PORT),
      state_(ServerState::STOPPED),
      serverTaskHandle_(nullptr),
      watchdogTaskHandle_(nullptr),
      startTime_(0),
      requestCount_(0),
      activeConnections_(0),
      lastWatchdogFeed_(0) {
    mutex_ = xSemaphoreCreateMutex();
}

WebServerCore::~WebServerCore() {
    stop();
    if (mutex_) {
        vSemaphoreDelete(mutex_);
    }
}

bool WebServerCore::init(uint16_t port) {
    xSemaphoreTake(mutex_, portMAX_DELAY);
    
    // Set port if provided
    if (port > 0) {
        port_ = port;
    }
    
    LOG_INFO("Core", "Initializing ESP32 Web Framework...");
    
    // Initialize template engine
    if (!Templates().init()) {
        LOG_WARN("Core", "Template engine init failed (non-fatal)");
    }
    
    // Initialize static handler
    if (!Static().init()) {
        LOG_WARN("Core", "Static handler init failed (non-fatal)");
    }
    
    // Initialize diagnostics
    if (settings::DIAGNOSTICS_ENABLED) {
        Diag().init();
        Diag().registerRoutes(Routes());
    }
    
    // Initialize modules
    for (auto& module : modules_) {
        if (module->init()) {
            module->registerRoutes(Routes());
            LOG_INFO("Core", String("Module loaded: ") + module->getName());
        } else {
            LOG_ERROR("Core", String("Module init failed: ") + module->getName());
        }
    }
    
    state_ = ServerState::STOPPED;
    LOG_INFO("Core", "Framework initialized");
    
    xSemaphoreGive(mutex_);
    return true;
}

bool WebServerCore::start() {
    xSemaphoreTake(mutex_, portMAX_DELAY);
    
    if (state_ == ServerState::RUNNING) {
        LOG_WARN("Core", "Server already running");
        xSemaphoreGive(mutex_);
        return true;
    }
    
    state_ = ServerState::STARTING;
    LOG_INFO("Core", "Starting web server...");
    
    // Start WiFi server
    server_.begin();
    
    // Record start time
    startTime_ = millis();
    lastWatchdogFeed_ = millis();
    
    // Create server task on core 1
    BaseType_t result = xTaskCreatePinnedToCore(
        serverTask,
        "WebServerTask",
        settings::SERVER_STACK_SIZE,
        this,
        settings::SERVER_TASK_PRIORITY,
        &serverTaskHandle_,
        settings::SERVER_CORE
    );
    
    if (result != pdPASS) {
        LOG_ERROR("Core", "Failed to create server task");
        state_ = ServerState::ERROR;
        xSemaphoreGive(mutex_);
        return false;
    }
    
    // Create watchdog task if enabled
    if (settings::WATCHDOG_ENABLED) {
        xTaskCreatePinnedToCore(
            watchdogTask,
            "WatchdogTask",
            2048,
            this,
            1,
            &watchdogTaskHandle_,
            0
        );
    }
    
    state_ = ServerState::RUNNING;
    LOG_INFO("Core", "Web server started on port " + String(settings::SERVER_PORT));
    
    xSemaphoreGive(mutex_);
    return true;
}

bool WebServerCore::stop() {
    xSemaphoreTake(mutex_, portMAX_DELAY);
    
    if (state_ != ServerState::RUNNING) {
        xSemaphoreGive(mutex_);
        return true;
    }
    
    state_ = ServerState::STOPPING;
    LOG_INFO("Core", "Stopping web server...");
    
    // Delete tasks
    if (serverTaskHandle_) {
        vTaskDelete(serverTaskHandle_);
        serverTaskHandle_ = nullptr;
    }
    
    if (watchdogTaskHandle_) {
        vTaskDelete(watchdogTaskHandle_);
        watchdogTaskHandle_ = nullptr;
    }
    
    // Stop server
    server_.stop();
    
    state_ = ServerState::STOPPED;
    LOG_INFO("Core", "Web server stopped");
    
    xSemaphoreGive(mutex_);
    return true;
}

bool WebServerCore::restart() {
    LOG_INFO("Core", "Restarting web server...");
    stop();
    vTaskDelay(pdMS_TO_TICKS(100));
    return start();
}

void WebServerCore::registerModule(std::unique_ptr<WebModule> module) {
    xSemaphoreTake(mutex_, portMAX_DELAY);
    modules_.push_back(std::move(module));
    xSemaphoreGive(mutex_);
}

uint32_t WebServerCore::getUptime() const {
    return millis() - startTime_;
}

void WebServerCore::feedWatchdog() {
    lastWatchdogFeed_ = millis();
}

void WebServerCore::serverTask(void* parameter) {
    WebServerCore* core = static_cast<WebServerCore*>(parameter);
    
    while (true) {
        // Check for new clients
        WiFiClient client = core->server_.available();
        
        if (client) {
            core->activeConnections_++;
            core->handleClient(client);
            core->activeConnections_--;
        }
        
        // Feed watchdog
        core->feedWatchdog();
        
        // Small delay to prevent task hogging
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

void WebServerCore::watchdogTask(void* parameter) {
    WebServerCore* core = static_cast<WebServerCore*>(parameter);
    
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        
        uint32_t elapsed = millis() - core->lastWatchdogFeed_;
        
        if (elapsed > settings::WATCHDOG_TIMEOUT_SECONDS * 1000) {
            LOG_ERROR("Core", "Watchdog timeout! Server may be hung.");
            
            if (settings::WATCHDOG_AUTO_RESTART) {
                LOG_INFO("Core", "Attempting auto-restart...");
                core->restart();
            }
        }
    }
}

void WebServerCore::handleClient(WiFiClient& client) {
    uint32_t startTime = millis();
    
    // Wait for data with timeout
    uint32_t timeout = millis() + settings::CONNECTION_TIMEOUT_MS;
    while (!client.available() && millis() < timeout) {
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    
    if (!client.available()) {
        client.stop();
        return;
    }
    
    // Parse request
    Request request;
    if (!parseRequest(client, request)) {
        Response errorResp = Response::error(HttpStatus::BAD_REQUEST);
        sendResponse(client, errorResp);
        client.stop();
        return;
    }
    
    // Get client info
    request.clientIP = client.remoteIP().toString();
    request.clientPort = client.remotePort();
    
    LOG_DEBUG("Core", String(methodToString(request.method)) + " " + request.path + 
              " from " + request.clientIP);
    
    // Initialize response
    Response response;
    
    // Run middleware (before)
    if (!middleware_.processBefore(request, response)) {
        sendResponse(client, response);
        client.stop();
        requestCount_++;
        return;
    }
    
    // Try to match static file first
    String staticPath;
    if (Routes().matchStatic(request.path, staticPath)) {
        response = Static().serve(staticPath, request);
    } else {
        // Match route
        std::map<String, String> params;
        const Route* route = Routes().match(request.method, request.path, params);
        
        if (route) {
            request.params = params;
            
            try {
                response = route->handler(request);
            } catch (...) {
                LOG_ERROR("Core", "Exception in view handler");
                response = Response::error(HttpStatus::INTERNAL_SERVER_ERROR);
            }
        } else {
            // Check if method not allowed
            bool methodNotAllowed = false;
            for (int m = 0; m < 7; m++) {
                HttpMethod method = static_cast<HttpMethod>(m);
                if (Routes().match(method, request.path, params)) {
                    methodNotAllowed = true;
                    break;
                }
            }
            
            if (methodNotAllowed) {
                response = Response::error(HttpStatus::METHOD_NOT_ALLOWED);
            } else {
                response = Response::error(HttpStatus::NOT_FOUND, 
                                          "The requested URL was not found on this server.");
            }
        }
    }
    
    // Run middleware (after)
    middleware_.processAfter(request, response);
    
    // Send response
    sendResponse(client, response);
    
    // Record diagnostics
    uint32_t responseTime = millis() - startTime;
    if (settings::DIAGNOSTICS_ENABLED) {
        Diag().recordRequest(request.path, methodToString(request.method),
                            responseTime, (int)response.status);
    }
    
    // Close connection
    client.stop();
    requestCount_++;
}

bool WebServerCore::parseRequest(WiFiClient& client, Request& request) {
    // Read first line (request line)
    String line = client.readStringUntil('\n');
    line.trim();
    
    if (line.isEmpty()) {
        return false;
    }
    
    // Parse request line: METHOD PATH HTTP/VERSION
    int firstSpace = line.indexOf(' ');
    int secondSpace = line.indexOf(' ', firstSpace + 1);
    
    if (firstSpace < 0 || secondSpace < 0) {
        return false;
    }
    
    String methodStr = line.substring(0, firstSpace);
    request.fullUrl = line.substring(firstSpace + 1, secondSpace);
    request.httpVersion = line.substring(secondSpace + 1);
    
    request.method = stringToMethod(methodStr);
    
    // Parse path and query string
    int queryStart = request.fullUrl.indexOf('?');
    if (queryStart > 0) {
        request.path = request.fullUrl.substring(0, queryStart);
        request.queryString = request.fullUrl.substring(queryStart + 1);
        
        // Parse query parameters
        String qs = request.queryString;
        while (qs.length() > 0) {
            int ampPos = qs.indexOf('&');
            String pair = ampPos > 0 ? qs.substring(0, ampPos) : qs;
            
            int eqPos = pair.indexOf('=');
            if (eqPos > 0) {
                String key = pair.substring(0, eqPos);
                String value = pair.substring(eqPos + 1);
                // URL decode would go here
                request.query[key] = value;
            }
            
            if (ampPos > 0) {
                qs = qs.substring(ampPos + 1);
            } else {
                break;
            }
        }
    } else {
        request.path = request.fullUrl;
    }
    
    // Read headers
    while (client.available()) {
        line = client.readStringUntil('\n');
        line.trim();
        
        if (line.isEmpty()) {
            break; // End of headers
        }
        
        int colonPos = line.indexOf(':');
        if (colonPos > 0) {
            String headerName = line.substring(0, colonPos);
            String headerValue = line.substring(colonPos + 1);
            headerValue.trim();
            request.headers[headerName] = headerValue;
            
            // Parse specific headers
            if (headerName.equalsIgnoreCase("Content-Length")) {
                request.contentLength = headerValue.toInt();
            } else if (headerName.equalsIgnoreCase("Content-Type")) {
                request.contentType = headerValue;
            } else if (headerName.equalsIgnoreCase("Connection")) {
                request.keepAlive = headerValue.equalsIgnoreCase("keep-alive");
            }
        }
    }
    
    // Read body if present
    if (request.contentLength > 0) {
        request.body.reserve(request.contentLength);
        
        size_t bytesRead = 0;
        while (bytesRead < request.contentLength && client.available()) {
            char c = client.read();
            request.body += c;
            bytesRead++;
        }
        
        // Parse form data if applicable
        if (request.contentType.startsWith("application/x-www-form-urlencoded")) {
            String formData = request.body;
            while (formData.length() > 0) {
                int ampPos = formData.indexOf('&');
                String pair = ampPos > 0 ? formData.substring(0, ampPos) : formData;
                
                int eqPos = pair.indexOf('=');
                if (eqPos > 0) {
                    String key = pair.substring(0, eqPos);
                    String value = pair.substring(eqPos + 1);
                    request.form[key] = value;
                }
                
                if (ampPos > 0) {
                    formData = formData.substring(ampPos + 1);
                } else {
                    break;
                }
            }
        }
    }
    
    return true;
}

void WebServerCore::sendResponse(WiFiClient& client, Response& response) {
    if (response.isStreaming && response.streamCallback) {
        // Streaming response
        String headers = "HTTP/1.1 " + String((int)response.status) + " " + 
                        statusToString(response.status) + "\r\n";
        headers += "Content-Type: " + response.contentType + "\r\n";
        headers += "Transfer-Encoding: chunked\r\n";
        
        for (const auto& header : response.headers) {
            headers += header.first + ": " + header.second + "\r\n";
        }
        
        headers += "Connection: close\r\n\r\n";
        client.print(headers);
        
        response.streamCallback(client);
        
        // Final chunk
        client.print("0\r\n\r\n");
    } else if (response.isFile) {
        // File response - handled by FileIO
        // For now, send error if file handling not implemented
        Response errorResp = Response::error(HttpStatus::NOT_IMPLEMENTED, 
                                            "File streaming not yet implemented");
        client.print(errorResp.build());
    } else {
        // Normal response
        client.print(response.build());
    }
    
    client.flush();
}

} // namespace espweb
