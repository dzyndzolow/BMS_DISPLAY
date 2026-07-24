/**
 * @file middleware.hpp
 * @brief ESP32 Web Framework - Middleware System
 * 
 * Middleware for request/response processing:
 * - Authentication
 * - Logging
 * - CORS
 * - Rate limiting
 * 
 * @author ESP32 Web Framework
 * @version 1.0.0
 */

#ifndef ESP_WEB_FRAMEWORK_MIDDLEWARE_HPP
#define ESP_WEB_FRAMEWORK_MIDDLEWARE_HPP

#include <Arduino.h>
#include <vector>
#include <map>
#include <memory>
#include <functional>

// Forward declarations
namespace espweb {
    class Request;
    class Response;
}

namespace espweb {

/**
 * @brief Middleware result
 */
enum class MiddlewareResult {
    CONTINUE,       ///< Continue to next middleware/view
    STOP,           ///< Stop processing, send response
    ERROR           ///< Error occurred
};

/**
 * @brief Base middleware class
 * 
 * Inherit from this class to create custom middleware.
 */
class Middleware {
public:
    virtual ~Middleware() = default;
    
    /**
     * @brief Process request before view
     * @param request Request object
     * @param response Response object (for early response)
     * @return MiddlewareResult indicating how to proceed
     */
    virtual MiddlewareResult processRequest(Request& request, Response& response) {
        return MiddlewareResult::CONTINUE;
    }
    
    /**
     * @brief Process response after view
     * @param request Request object
     * @param response Response object
     * @return MiddlewareResult indicating how to proceed
     */
    virtual MiddlewareResult processResponse(Request& request, Response& response) {
        return MiddlewareResult::CONTINUE;
    }
    
    /**
     * @brief Get middleware name
     * @return Middleware name string
     */
    virtual const char* getName() const = 0;
    
    /**
     * @brief Get middleware priority (lower = earlier)
     * @return Priority value
     */
    virtual int getPriority() const { return 100; }
};

/**
 * @brief Middleware manager
 * 
 * Manages middleware chain execution.
 */
class MiddlewareManager {
public:
    MiddlewareManager() = default;
    ~MiddlewareManager() = default;
    
    /**
     * @brief Add middleware to chain
     * @param middleware Unique pointer to middleware
     */
    void add(std::unique_ptr<Middleware> middleware);
    
    /**
     * @brief Remove middleware by name
     * @param name Middleware name
     * @return true if removed
     */
    bool remove(const String& name);
    
    /**
     * @brief Process all middleware before view
     * @param request Request object
     * @param response Response object
     * @return true if processing should continue to view
     */
    bool processBefore(Request& request, Response& response);
    
    /**
     * @brief Process all middleware after view
     * @param request Request object
     * @param response Response object
     * @return true if response should be sent
     */
    bool processAfter(Request& request, Response& response);
    
    /**
     * @brief Get middleware count
     * @return Number of middleware
     */
    size_t count() const { return middlewares_.size(); }
    
    /**
     * @brief Clear all middleware
     */
    void clear() { middlewares_.clear(); }
    
private:
    std::vector<std::unique_ptr<Middleware>> middlewares_;
};

//==============================================================================
// Built-in Middleware
//==============================================================================

/**
 * @brief Logging middleware
 * 
 * Logs all requests to Serial and optionally to file.
 */
class LoggingMiddleware : public Middleware {
public:
    LoggingMiddleware(bool verbose = false) : verbose_(verbose) {}
    
    MiddlewareResult processRequest(Request& request, Response& response) override;
    MiddlewareResult processResponse(Request& request, Response& response) override;
    
    const char* getName() const override { return "LoggingMiddleware"; }
    int getPriority() const override { return 10; }
    
private:
    bool verbose_;
    uint32_t requestStart_ = 0;
};

/**
 * @brief CORS middleware
 * 
 * Handles Cross-Origin Resource Sharing headers.
 */
class CorsMiddleware : public Middleware {
public:
    CorsMiddleware(const String& allowedOrigins = "*",
                   const String& allowedMethods = "GET, POST, PUT, DELETE, OPTIONS",
                   const String& allowedHeaders = "Content-Type, Authorization");
    
    MiddlewareResult processRequest(Request& request, Response& response) override;
    MiddlewareResult processResponse(Request& request, Response& response) override;
    
    const char* getName() const override { return "CorsMiddleware"; }
    int getPriority() const override { return 20; }
    
private:
    String allowedOrigins_;
    String allowedMethods_;
    String allowedHeaders_;
};

/**
 * @brief Rate limiting middleware
 * 
 * Limits requests per IP address.
 */
class RateLimitMiddleware : public Middleware {
public:
    RateLimitMiddleware(uint16_t maxRequests = 60, uint32_t windowMs = 60000);
    
    MiddlewareResult processRequest(Request& request, Response& response) override;
    
    const char* getName() const override { return "RateLimitMiddleware"; }
    int getPriority() const override { return 5; }
    
private:
    struct ClientInfo {
        uint32_t firstRequest;
        uint16_t count;
    };
    
    uint16_t maxRequests_;
    uint32_t windowMs_;
    std::map<String, ClientInfo> clients_;
    
    void cleanup();
};

/**
 * @brief Basic auth middleware
 * 
 * HTTP Basic Authentication.
 */
class BasicAuthMiddleware : public Middleware {
public:
    BasicAuthMiddleware(const String& username, const String& password,
                        const String& realm = "ESP32 Web Framework");
    
    MiddlewareResult processRequest(Request& request, Response& response) override;
    
    const char* getName() const override { return "BasicAuthMiddleware"; }
    int getPriority() const override { return 15; }
    
private:
    String username_;
    String password_;
    String realm_;
    
    bool checkAuth(const String& authHeader);
};

/**
 * @brief API Key middleware
 * 
 * API Key authentication via header or query parameter.
 */
class ApiKeyMiddleware : public Middleware {
public:
    ApiKeyMiddleware(const String& apiKey, 
                     const String& headerName = "X-API-Key",
                     const String& queryParam = "api_key");
    
    MiddlewareResult processRequest(Request& request, Response& response) override;
    
    const char* getName() const override { return "ApiKeyMiddleware"; }
    int getPriority() const override { return 15; }
    
private:
    String apiKey_;
    String headerName_;
    String queryParam_;
};

} // namespace espweb

#endif // ESP_WEB_FRAMEWORK_MIDDLEWARE_HPP
