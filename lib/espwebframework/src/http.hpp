/**
 * @file http.hpp
 * @brief ESP32 Web Framework - HTTP Request/Response Classes
 * 
 * Contains Request and Response classes for HTTP handling.
 * 
 * @author ESP32 Web Framework
 * @version 1.0.0
 */

#ifndef ESP_WEB_FRAMEWORK_HTTP_HPP
#define ESP_WEB_FRAMEWORK_HTTP_HPP

#include <Arduino.h>
#include <WiFi.h>
#include <map>
#include <vector>
#include <string>
#include <functional>
#include <ArduinoJson.h>

namespace espweb {

/**
 * @brief HTTP methods enumeration
 */
enum class HttpMethod {
    GET,
    POST,
    PUT,
    DELETE,
    PATCH,
    HEAD,
    OPTIONS,
    UNKNOWN
};

/**
 * @brief HTTP status codes
 */
enum class HttpStatus {
    OK = 200,
    CREATED = 201,
    NO_CONTENT = 204,
    MOVED_PERMANENTLY = 301,
    FOUND = 302,
    NOT_MODIFIED = 304,
    BAD_REQUEST = 400,
    UNAUTHORIZED = 401,
    FORBIDDEN = 403,
    NOT_FOUND = 404,
    METHOD_NOT_ALLOWED = 405,
    INTERNAL_SERVER_ERROR = 500,
    NOT_IMPLEMENTED = 501,
    SERVICE_UNAVAILABLE = 503
};

/**
 * @brief Convert HttpMethod to string
 */
inline const char* methodToString(HttpMethod method) {
    switch (method) {
        case HttpMethod::GET: return "GET";
        case HttpMethod::POST: return "POST";
        case HttpMethod::PUT: return "PUT";
        case HttpMethod::DELETE: return "DELETE";
        case HttpMethod::PATCH: return "PATCH";
        case HttpMethod::HEAD: return "HEAD";
        case HttpMethod::OPTIONS: return "OPTIONS";
        default: return "UNKNOWN";
    }
}

/**
 * @brief Convert string to HttpMethod
 */
inline HttpMethod stringToMethod(const String& method) {
    if (method == "GET") return HttpMethod::GET;
    if (method == "POST") return HttpMethod::POST;
    if (method == "PUT") return HttpMethod::PUT;
    if (method == "DELETE") return HttpMethod::DELETE;
    if (method == "PATCH") return HttpMethod::PATCH;
    if (method == "HEAD") return HttpMethod::HEAD;
    if (method == "OPTIONS") return HttpMethod::OPTIONS;
    return HttpMethod::UNKNOWN;
}

/**
 * @brief Get status text for HTTP status code
 */
inline const char* statusToString(HttpStatus status) {
    switch (status) {
        case HttpStatus::OK: return "OK";
        case HttpStatus::CREATED: return "Created";
        case HttpStatus::NO_CONTENT: return "No Content";
        case HttpStatus::MOVED_PERMANENTLY: return "Moved Permanently";
        case HttpStatus::FOUND: return "Found";
        case HttpStatus::NOT_MODIFIED: return "Not Modified";
        case HttpStatus::BAD_REQUEST: return "Bad Request";
        case HttpStatus::UNAUTHORIZED: return "Unauthorized";
        case HttpStatus::FORBIDDEN: return "Forbidden";
        case HttpStatus::NOT_FOUND: return "Not Found";
        case HttpStatus::METHOD_NOT_ALLOWED: return "Method Not Allowed";
        case HttpStatus::INTERNAL_SERVER_ERROR: return "Internal Server Error";
        case HttpStatus::NOT_IMPLEMENTED: return "Not Implemented";
        case HttpStatus::SERVICE_UNAVAILABLE: return "Service Unavailable";
        default: return "Unknown";
    }
}

/**
 * @brief Uploaded file information
 */
struct UploadedFile {
    String name;            ///< Form field name
    String filename;        ///< Original filename
    String contentType;     ///< MIME type
    String tempPath;        ///< Temporary file path
    size_t size;            ///< File size in bytes
};

/**
 * @brief HTTP Request class
 * 
 * Contains all information about an incoming HTTP request.
 */
class Request {
public:
    Request() = default;
    ~Request() = default;
    
    // Request line
    HttpMethod method = HttpMethod::GET;    ///< HTTP method
    String path;                             ///< Request path
    String fullUrl;                          ///< Full URL with query string
    String queryString;                      ///< Raw query string
    String httpVersion;                      ///< HTTP version
    
    // Headers
    std::map<String, String> headers;        ///< Request headers
    
    // URL Parameters (from router)
    std::map<String, String> params;         ///< URL parameters (e.g., /user/<id>)
    
    // Query Parameters
    std::map<String, String> query;          ///< Query string parameters
    
    // Form data (POST)
    std::map<String, String> form;           ///< Form POST data
    
    // Uploaded files
    std::vector<UploadedFile> files;         ///< Uploaded files
    
    // Raw body
    String body;                             ///< Raw request body
    
    // Client info
    String clientIP;                         ///< Client IP address
    uint16_t clientPort;                     ///< Client port
    
    // Metadata
    bool keepAlive = false;                  ///< Keep-alive connection
    size_t contentLength = 0;                ///< Content-Length header value
    String contentType;                      ///< Content-Type header value
    
    /**
     * @brief Get header value
     * @param name Header name (case-insensitive)
     * @return Header value or empty string
     */
    String getHeader(const String& name) const;
    
    /**
     * @brief Get query parameter
     * @param name Parameter name
     * @param defaultValue Default value if not found
     * @return Parameter value
     */
    String getQuery(const String& name, const String& defaultValue = "") const;
    
    /**
     * @brief Get URL parameter
     * @param name Parameter name
     * @param defaultValue Default value if not found
     * @return Parameter value
     */
    String getParam(const String& name, const String& defaultValue = "") const;
    
    /**
     * @brief Get form field value
     * @param name Field name
     * @param defaultValue Default value if not found
     * @return Field value
     */
    String getForm(const String& name, const String& defaultValue = "") const;
    
    /**
     * @brief Check if request is AJAX
     * @return true if X-Requested-With: XMLHttpRequest
     */
    bool isAjax() const;
    
    /**
     * @brief Shorthand for getParam
     * @param name Parameter name
     * @return Parameter value
     */
    String param(const String& name) const { return getParam(name); }
    
    /**
     * @brief Shorthand for getForm (getBody alias)
     * @param name Field name
     * @return Field value
     */
    String getBody(const String& name) const { return getForm(name); }
    
    /**
     * @brief Parse body as JSON
     * @param doc JsonDocument to populate
     * @return true if parsing successful
     */
    bool parseJson(JsonDocument& doc) const;
};

/**
 * @brief HTTP Response class
 * 
 * Used to build and send HTTP responses.
 */
class Response {
public:
    Response() = default;
    ~Response() = default;
    
    // Status
    HttpStatus status = HttpStatus::OK;      ///< HTTP status code
    
    // Headers
    std::map<String, String> headers;        ///< Response headers
    
    // Body
    String body;                             ///< Response body
    
    // Content type
    String contentType = "text/html";        ///< Content-Type header
    
    // Streaming
    bool isStreaming = false;                ///< Is streaming response
    std::function<void(WiFiClient&)> streamCallback;  ///< Stream callback
    
    // File response
    bool isFile = false;                     ///< Is file response
    String filePath;                         ///< File path for file response
    
    /**
     * @brief Set response header
     * @param name Header name
     * @param value Header value
     * @return Reference to this for chaining
     */
    Response& setHeader(const String& name, const String& value);
    
    /**
     * @brief Set Content-Type header
     * @param type MIME type
     * @return Reference to this for chaining
     */
    Response& setContentType(const String& type);
    
    /**
     * @brief Set response status
     * @param code HTTP status code
     * @return Reference to this for chaining
     */
    Response& setStatus(HttpStatus code);
    
    /**
     * @brief Create HTML response
     * @param html HTML content
     * @param status HTTP status code
     * @return Response object
     */
    static Response html(const String& html, HttpStatus status = HttpStatus::OK);
    
    /**
     * @brief Create JSON response
     * @param doc JsonDocument containing data
     * @param status HTTP status code
     * @return Response object
     */
    static Response json(const JsonDocument& doc, HttpStatus status = HttpStatus::OK);
    
    /**
     * @brief Create JSON response from string
     * @param jsonStr JSON string
     * @param status HTTP status code
     * @return Response object
     */
    static Response json(const String& jsonStr, HttpStatus status = HttpStatus::OK);
    
    /**
     * @brief Create plain text response
     * @param text Text content
     * @param status HTTP status code
     * @return Response object
     */
    static Response text(const String& text, HttpStatus status = HttpStatus::OK);
    
    /**
     * @brief Create file download response
     * @param path File path
     * @param filename Download filename (optional)
     * @return Response object
     */
    static Response file(const String& path, const String& filename = "");
    
    /**
     * @brief Create redirect response
     * @param url Redirect URL
     * @param permanent Permanent (301) or temporary (302)
     * @return Response object
     */
    static Response redirect(const String& url, bool permanent = false);
    
    /**
     * @brief Create streaming response
     * @param callback Stream callback function
     * @param contentType Content type
     * @return Response object
     */
    static Response stream(std::function<void(WiFiClient&)> callback, 
                          const String& contentType = "application/octet-stream");
    
    /**
     * @brief Create error response
     * @param status HTTP status code
     * @param message Error message
     * @return Response object
     */
    static Response error(HttpStatus status, const String& message = "");
    
    /**
     * @brief Build raw HTTP response string
     * @return HTTP response as string
     */
    String build() const;
};

// Forward declaration
class Request;
class Response;

/**
 * @brief View handler function type
 */
using ViewHandler = std::function<Response(Request&)>;

} // namespace espweb

#endif // ESP_WEB_FRAMEWORK_HTTP_HPP
