/**
 * @file middleware.cpp
 * @brief ESP32 Web Framework - Middleware Implementation
 * 
 * @author ESP32 Web Framework
 * @version 1.0.0
 */

#include "middleware.hpp"
#include "http.hpp"
#include "logger.hpp"
#include <algorithm>

namespace espweb {

// Simple base64 decoder for Basic Auth
static String decodeBase64(const String& encoded) {
    static const char* b64chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    
    String result;
    uint8_t sextet[4];
    int j = 0;
    
    for (size_t i = 0; i < encoded.length(); i++) {
        char c = encoded[i];
        if (c == '=') break;
        
        const char* p = strchr(b64chars, c);
        if (!p) continue;
        
        sextet[j++] = p - b64chars;
        
        if (j == 4) {
            result += (char)((sextet[0] << 2) | (sextet[1] >> 4));
            result += (char)(((sextet[1] & 0x0F) << 4) | (sextet[2] >> 2));
            result += (char)(((sextet[2] & 0x03) << 6) | sextet[3]);
            j = 0;
        }
    }
    
    if (j >= 2) {
        result += (char)((sextet[0] << 2) | (sextet[1] >> 4));
        if (j >= 3) {
            result += (char)(((sextet[1] & 0x0F) << 4) | (sextet[2] >> 2));
        }
    }
    
    return result;
}

//==============================================================================
// MiddlewareManager Implementation
//==============================================================================

void MiddlewareManager::add(std::unique_ptr<Middleware> middleware) {
    middlewares_.push_back(std::move(middleware));
    
    // Sort by priority
    std::sort(middlewares_.begin(), middlewares_.end(),
        [](const std::unique_ptr<Middleware>& a, const std::unique_ptr<Middleware>& b) {
            return a->getPriority() < b->getPriority();
        });
}

bool MiddlewareManager::remove(const String& name) {
    for (auto it = middlewares_.begin(); it != middlewares_.end(); ++it) {
        if (String((*it)->getName()) == name) {
            middlewares_.erase(it);
            return true;
        }
    }
    return false;
}

bool MiddlewareManager::processBefore(Request& request, Response& response) {
    for (auto& middleware : middlewares_) {
        MiddlewareResult result = middleware->processRequest(request, response);
        
        if (result == MiddlewareResult::STOP) {
            return false;
        } else if (result == MiddlewareResult::ERROR) {
            response = Response::error(HttpStatus::INTERNAL_SERVER_ERROR);
            return false;
        }
    }
    return true;
}

bool MiddlewareManager::processAfter(Request& request, Response& response) {
    // Process in reverse order
    for (auto it = middlewares_.rbegin(); it != middlewares_.rend(); ++it) {
        MiddlewareResult result = (*it)->processResponse(request, response);
        
        if (result == MiddlewareResult::ERROR) {
            return false;
        }
    }
    return true;
}

//==============================================================================
// LoggingMiddleware Implementation
//==============================================================================

MiddlewareResult LoggingMiddleware::processRequest(Request& request, Response& response) {
    requestStart_ = millis();
    
    if (verbose_) {
        LOG_DEBUG("Request", String(methodToString(request.method)) + " " + 
                  request.path + " from " + request.clientIP);
    }
    
    return MiddlewareResult::CONTINUE;
}

MiddlewareResult LoggingMiddleware::processResponse(Request& request, Response& response) {
    uint32_t duration = millis() - requestStart_;
    
    String logMsg = request.clientIP + " - " + 
                    String(methodToString(request.method)) + " " + 
                    request.path + " " + 
                    String((int)response.status) + " " +
                    String(duration) + "ms";
    
    if ((int)response.status >= 400) {
        LOG_WARN("Access", logMsg);
    } else {
        LOG_INFO("Access", logMsg);
    }
    
    return MiddlewareResult::CONTINUE;
}

//==============================================================================
// CorsMiddleware Implementation
//==============================================================================

CorsMiddleware::CorsMiddleware(const String& allowedOrigins,
                               const String& allowedMethods,
                               const String& allowedHeaders)
    : allowedOrigins_(allowedOrigins),
      allowedMethods_(allowedMethods),
      allowedHeaders_(allowedHeaders) {
}

MiddlewareResult CorsMiddleware::processRequest(Request& request, Response& response) {
    // Handle preflight OPTIONS request
    if (request.method == HttpMethod::OPTIONS) {
        response.setStatus(HttpStatus::NO_CONTENT);
        response.setHeader("Access-Control-Allow-Origin", allowedOrigins_);
        response.setHeader("Access-Control-Allow-Methods", allowedMethods_);
        response.setHeader("Access-Control-Allow-Headers", allowedHeaders_);
        response.setHeader("Access-Control-Max-Age", "86400");
        return MiddlewareResult::STOP;
    }
    
    return MiddlewareResult::CONTINUE;
}

MiddlewareResult CorsMiddleware::processResponse(Request& request, Response& response) {
    response.setHeader("Access-Control-Allow-Origin", allowedOrigins_);
    
    if (!allowedHeaders_.isEmpty()) {
        response.setHeader("Access-Control-Allow-Headers", allowedHeaders_);
    }
    
    return MiddlewareResult::CONTINUE;
}

//==============================================================================
// RateLimitMiddleware Implementation
//==============================================================================

RateLimitMiddleware::RateLimitMiddleware(uint16_t maxRequests, uint32_t windowMs)
    : maxRequests_(maxRequests), windowMs_(windowMs) {
}

MiddlewareResult RateLimitMiddleware::processRequest(Request& request, Response& response) {
    cleanup();
    
    uint32_t now = millis();
    String clientIP = request.clientIP;
    
    auto it = clients_.find(clientIP);
    
    if (it == clients_.end()) {
        // New client
        clients_[clientIP] = { now, 1 };
    } else {
        // Existing client
        if (now - it->second.firstRequest > windowMs_) {
            // Window expired, reset
            it->second.firstRequest = now;
            it->second.count = 1;
        } else {
            it->second.count++;
            
            if (it->second.count > maxRequests_) {
                response = Response::error(HttpStatus::SERVICE_UNAVAILABLE,
                                          "Rate limit exceeded. Please try again later.");
                response.setHeader("Retry-After", String(windowMs_ / 1000));
                return MiddlewareResult::STOP;
            }
        }
    }
    
    // Add rate limit headers
    response.setHeader("X-RateLimit-Limit", String(maxRequests_));
    response.setHeader("X-RateLimit-Remaining", 
                       String(maxRequests_ - clients_[clientIP].count));
    
    return MiddlewareResult::CONTINUE;
}

void RateLimitMiddleware::cleanup() {
    uint32_t now = millis();
    
    for (auto it = clients_.begin(); it != clients_.end();) {
        if (now - it->second.firstRequest > windowMs_ * 2) {
            it = clients_.erase(it);
        } else {
            ++it;
        }
    }
}

//==============================================================================
// BasicAuthMiddleware Implementation
//==============================================================================

BasicAuthMiddleware::BasicAuthMiddleware(const String& username, 
                                         const String& password,
                                         const String& realm)
    : username_(username), password_(password), realm_(realm) {
}

MiddlewareResult BasicAuthMiddleware::processRequest(Request& request, Response& response) {
    String authHeader = request.getHeader("Authorization");
    
    if (authHeader.isEmpty() || !checkAuth(authHeader)) {
        response.setStatus(HttpStatus::UNAUTHORIZED);
        response.setHeader("WWW-Authenticate", "Basic realm=\"" + realm_ + "\"");
        response.body = "Unauthorized";
        return MiddlewareResult::STOP;
    }
    
    return MiddlewareResult::CONTINUE;
}

bool BasicAuthMiddleware::checkAuth(const String& authHeader) {
    if (!authHeader.startsWith("Basic ")) {
        return false;
    }
    
    String encoded = authHeader.substring(6);
    String decoded = decodeBase64(encoded);
    
    int colonPos = decoded.indexOf(':');
    if (colonPos < 0) {
        return false;
    }
    
    String user = decoded.substring(0, colonPos);
    String pass = decoded.substring(colonPos + 1);
    
    return (user == username_ && pass == password_);
}

//==============================================================================
// ApiKeyMiddleware Implementation
//==============================================================================

ApiKeyMiddleware::ApiKeyMiddleware(const String& apiKey,
                                   const String& headerName,
                                   const String& queryParam)
    : apiKey_(apiKey), headerName_(headerName), queryParam_(queryParam) {
}

MiddlewareResult ApiKeyMiddleware::processRequest(Request& request, Response& response) {
    // Check header first
    String headerKey = request.getHeader(headerName_);
    if (!headerKey.isEmpty() && headerKey == apiKey_) {
        return MiddlewareResult::CONTINUE;
    }
    
    // Check query parameter
    String queryKey = request.getQuery(queryParam_);
    if (!queryKey.isEmpty() && queryKey == apiKey_) {
        return MiddlewareResult::CONTINUE;
    }
    
    response = Response::error(HttpStatus::UNAUTHORIZED, "Invalid or missing API key");
    return MiddlewareResult::STOP;
}

} // namespace espweb
