/**
 * @file static.hpp
 * @brief ESP32 Web Framework - Static File Handler
 * 
 * Handles static file serving:
 * - MIME type detection
 * - Caching
 * - GZIP compression
 * - Range requests
 * 
 * @author ESP32 Web Framework
 * @version 1.0.0
 */

#ifndef ESP_WEB_FRAMEWORK_STATIC_HPP
#define ESP_WEB_FRAMEWORK_STATIC_HPP

#include <Arduino.h>
#include <map>
#include <memory>

#include "http.hpp"
#include "settings.h"

namespace espweb {

/**
 * @brief Cached static file
 */
struct CachedFile {
    String path;
    String mimeType;
    std::unique_ptr<uint8_t[]> data;
    size_t size;
    uint32_t cachedAt;
    String etag;
    bool gzipped = false;
};

/**
 * @brief Static file handler - Singleton
 * 
 * Serves static files from SPIFFS/SD with caching support.
 */
class StaticHandler {
public:
    /**
     * @brief Get singleton instance
     */
    static StaticHandler& getInstance();
    
    // Delete copy constructor and assignment
    StaticHandler(const StaticHandler&) = delete;
    StaticHandler& operator=(const StaticHandler&) = delete;
    
    /**
     * @brief Initialize static handler
     * @param staticDir Directory for static files
     * @param urlPath URL path prefix for static files
     * @return true if successful
     */
    bool init(const String& staticDir = "/static", const String& urlPath = "/static");
    
    /**
     * @brief Serve static file
     * @param path File path
     * @param request Request object
     * @return Response with file content
     */
    Response serve(const String& path, Request& request);
    
    /**
     * @brief Check if file exists
     * @param path File path
     * @return true if file exists
     */
    bool exists(const String& path);
    
    /**
     * @brief Get MIME type for file
     * @param path File path
     * @return MIME type string
     */
    String getMimeType(const String& path);
    
    /**
     * @brief Preload file into cache
     * @param path File path
     * @return true if successful
     */
    bool preload(const String& path);
    
    /**
     * @brief Clear file cache
     */
    void clearCache();
    
    /**
     * @brief Remove file from cache
     * @param path File path
     */
    void invalidate(const String& path);
    
    /**
     * @brief Get cache statistics
     * @param size Current cache size in bytes
     * @param count Number of cached files
     * @param hits Cache hits
     * @param misses Cache misses
     */
    void getCacheStats(size_t& size, size_t& count, 
                       uint32_t& hits, uint32_t& misses) const;
    
    /**
     * @brief Register custom MIME type
     * @param extension File extension (without dot)
     * @param mimeType MIME type string
     */
    void registerMimeType(const String& extension, const String& mimeType);
    
private:
    StaticHandler();
    ~StaticHandler() = default;
    
    /**
     * @brief Load file from filesystem
     * @param path File path
     * @param data Output data buffer
     * @param size Output file size
     * @return true if successful
     */
    bool loadFile(const String& path, std::unique_ptr<uint8_t[]>& data, size_t& size);
    
    /**
     * @brief Check if file should be cached
     * @param path File path
     * @param size File size
     * @return true if file should be cached
     */
    bool shouldCache(const String& path, size_t size);
    
    /**
     * @brief Calculate ETag for file
     * @param path File path
     * @param size File size
     * @param modTime Modification time
     * @return ETag string
     */
    String calculateEtag(const String& path, size_t size, uint32_t modTime);
    
    /**
     * @brief Check if client cache is valid
     * @param request Request object
     * @param etag Server ETag
     * @return true if client cache is valid
     */
    bool isClientCacheValid(const Request& request, const String& etag);
    
    /**
     * @brief Handle range request
     * @param request Request object
     * @param path File path
     * @param size File size
     * @return Response with partial content
     */
    Response handleRangeRequest(const Request& request, const String& path, size_t size);
    
    /**
     * @brief Cleanup old cache entries
     */
    void cleanupCache();
    
    std::map<String, std::unique_ptr<CachedFile>> cache_;
    std::map<String, String> mimeTypes_;
    
    String staticDir_;
    String urlPath_;
    
    size_t currentCacheSize_ = 0;
    uint32_t cacheHits_ = 0;
    uint32_t cacheMisses_ = 0;
    
    SemaphoreHandle_t mutex_;
};

/**
 * @brief Convenience function to get static handler
 */
inline StaticHandler& Static() {
    return StaticHandler::getInstance();
}

//==============================================================================
// MIME Types
//==============================================================================

namespace mime {

// Text
constexpr const char* HTML = "text/html";
constexpr const char* CSS = "text/css";
constexpr const char* JS = "application/javascript";
constexpr const char* JSON = "application/json";
constexpr const char* XML = "application/xml";
constexpr const char* TXT = "text/plain";
constexpr const char* CSV = "text/csv";

// Images
constexpr const char* PNG = "image/png";
constexpr const char* JPG = "image/jpeg";
constexpr const char* GIF = "image/gif";
constexpr const char* SVG = "image/svg+xml";
constexpr const char* ICO = "image/x-icon";
constexpr const char* WEBP = "image/webp";

// Fonts
constexpr const char* WOFF = "font/woff";
constexpr const char* WOFF2 = "font/woff2";
constexpr const char* TTF = "font/ttf";
constexpr const char* OTF = "font/otf";
constexpr const char* EOT = "application/vnd.ms-fontobject";

// Binary
constexpr const char* PDF = "application/pdf";
constexpr const char* ZIP = "application/zip";
constexpr const char* GZIP = "application/gzip";
constexpr const char* BINARY = "application/octet-stream";

// Media
constexpr const char* MP3 = "audio/mpeg";
constexpr const char* WAV = "audio/wav";
constexpr const char* MP4 = "video/mp4";
constexpr const char* WEBM = "video/webm";

} // namespace mime

/**
 * @brief Helper function to get StaticHandler singleton
 */
inline StaticHandler& StaticFiles() {
    return StaticHandler::getInstance();
}

/**
 * @brief Helper function to serve static files from a directory
 * Registers routes for serving static files
 * @param urlPrefix URL prefix for static files (e.g., "/static")
 * @param dirPath Directory path on filesystem
 */
inline void ServeStatic(const String& urlPrefix, const String& dirPath) {
    StaticFiles().init(dirPath, urlPrefix);
}

} // namespace espweb

#endif // ESP_WEB_FRAMEWORK_STATIC_HPP
