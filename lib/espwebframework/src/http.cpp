/**
 * @file http.cpp
 * @brief ESP32 Web Framework - HTTP Request/Response Implementation
 * 
 * @author ESP32 Web Framework
 * @version 1.0.0
 */

#include "http.hpp"

namespace espweb {

//==============================================================================
// Request Implementation
//==============================================================================

String Request::getHeader(const String& name) const {
    // Case-insensitive header lookup
    String lowerName = name;
    lowerName.toLowerCase();
    
    for (const auto& header : headers) {
        String headerKey = header.first;
        headerKey.toLowerCase();
        if (headerKey == lowerName) {
            return header.second;
        }
    }
    return "";
}

String Request::getQuery(const String& name, const String& defaultValue) const {
    auto it = query.find(name);
    if (it != query.end()) {
        return it->second;
    }
    return defaultValue;
}

String Request::getParam(const String& name, const String& defaultValue) const {
    auto it = params.find(name);
    if (it != params.end()) {
        return it->second;
    }
    return defaultValue;
}

String Request::getForm(const String& name, const String& defaultValue) const {
    auto it = form.find(name);
    if (it != form.end()) {
        return it->second;
    }
    return defaultValue;
}

bool Request::isAjax() const {
    String xrw = getHeader("X-Requested-With");
    return xrw.equalsIgnoreCase("XMLHttpRequest");
}

bool Request::parseJson(JsonDocument& doc) const {
    if (body.isEmpty()) {
        return false;
    }
    DeserializationError error = deserializeJson(doc, body);
    return error == DeserializationError::Ok;
}

//==============================================================================
// Response Implementation
//==============================================================================

Response& Response::setHeader(const String& name, const String& value) {
    headers[name] = value;
    return *this;
}

Response& Response::setContentType(const String& type) {
    contentType = type;
    return *this;
}

Response& Response::setStatus(HttpStatus code) {
    status = code;
    return *this;
}

Response Response::html(const String& html, HttpStatus status) {
    Response resp;
    resp.status = status;
    resp.contentType = "text/html; charset=utf-8";
    resp.body = html;
    return resp;
}

Response Response::json(const JsonDocument& doc, HttpStatus status) {
    Response resp;
    resp.status = status;
    resp.contentType = "application/json";
    serializeJson(doc, resp.body);
    return resp;
}

Response Response::json(const String& jsonStr, HttpStatus status) {
    Response resp;
    resp.status = status;
    resp.contentType = "application/json";
    resp.body = jsonStr;
    return resp;
}

Response Response::text(const String& text, HttpStatus status) {
    Response resp;
    resp.status = status;
    resp.contentType = "text/plain; charset=utf-8";
    resp.body = text;
    return resp;
}

Response Response::file(const String& path, const String& filename) {
    Response resp;
    resp.isFile = true;
    resp.filePath = path;
    
    String downloadName = filename.isEmpty() ? 
                          path.substring(path.lastIndexOf('/') + 1) : filename;
    
    resp.setHeader("Content-Disposition", 
                   "attachment; filename=\"" + downloadName + "\"");
    return resp;
}

Response Response::redirect(const String& url, bool permanent) {
    Response resp;
    resp.status = permanent ? HttpStatus::MOVED_PERMANENTLY : HttpStatus::FOUND;
    resp.setHeader("Location", url);
    resp.body = "";
    return resp;
}

Response Response::stream(std::function<void(WiFiClient&)> callback, 
                          const String& contentType) {
    Response resp;
    resp.isStreaming = true;
    resp.streamCallback = callback;
    resp.contentType = contentType;
    return resp;
}

Response Response::error(HttpStatus status, const String& message) {
    Response resp;
    resp.status = status;
    resp.contentType = "text/html; charset=utf-8";
    
    String statusText = statusToString(status);
    String msg = message.isEmpty() ? statusText : message;
    
    resp.body = "<!DOCTYPE html><html><head><title>";
    resp.body += String((int)status) + " " + statusText;
    resp.body += "</title><style>";
    resp.body += "body{font-family:Arial,sans-serif;text-align:center;padding:50px;}";
    resp.body += "h1{color:#333;} p{color:#666;}";
    resp.body += "</style></head><body>";
    resp.body += "<h1>" + String((int)status) + " " + statusText + "</h1>";
    resp.body += "<p>" + msg + "</p>";
    resp.body += "</body></html>";
    
    return resp;
}

String Response::build() const {
    String response;
    
    // Status line
    response += "HTTP/1.1 ";
    response += String((int)status);
    response += " ";
    response += statusToString(status);
    response += "\r\n";
    
    // Content-Type header
    response += "Content-Type: ";
    response += contentType;
    response += "\r\n";
    
    // Content-Length header
    if (!isStreaming && !isFile) {
        response += "Content-Length: ";
        response += String(body.length());
        response += "\r\n";
    }
    
    // Custom headers
    for (const auto& header : headers) {
        response += header.first;
        response += ": ";
        response += header.second;
        response += "\r\n";
    }
    
    // Connection header
    response += "Connection: close\r\n";
    
    // Server header
    response += "Server: ESP32-WebFramework/1.0\r\n";
    
    // End of headers
    response += "\r\n";
    
    // Body (if not streaming or file)
    if (!isStreaming && !isFile) {
        response += body;
    }
    
    return response;
}

} // namespace espweb
