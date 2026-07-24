/**
 * @file static.cpp
 * @brief ESP32 Web Framework - Static File Handler Implementation
 * 
 * @author ESP32 Web Framework
 * @version 1.0.0
 */

#include "static.hpp"
#include "fileio.hpp"
#include "logger.hpp"

namespace espweb {

// Singleton instance
StaticHandler& StaticHandler::getInstance() {
    static StaticHandler instance;
    return instance;
}

StaticHandler::StaticHandler() {
    mutex_ = xSemaphoreCreateMutex();
    
    // Register default MIME types
    mimeTypes_["html"] = mime::HTML;
    mimeTypes_["htm"] = mime::HTML;
    mimeTypes_["css"] = mime::CSS;
    mimeTypes_["js"] = mime::JS;
    mimeTypes_["json"] = mime::JSON;
    mimeTypes_["xml"] = mime::XML;
    mimeTypes_["txt"] = mime::TXT;
    mimeTypes_["csv"] = mime::CSV;
    
    mimeTypes_["png"] = mime::PNG;
    mimeTypes_["jpg"] = mime::JPG;
    mimeTypes_["jpeg"] = mime::JPG;
    mimeTypes_["gif"] = mime::GIF;
    mimeTypes_["svg"] = mime::SVG;
    mimeTypes_["ico"] = mime::ICO;
    mimeTypes_["webp"] = mime::WEBP;
    
    mimeTypes_["woff"] = mime::WOFF;
    mimeTypes_["woff2"] = mime::WOFF2;
    mimeTypes_["ttf"] = mime::TTF;
    mimeTypes_["otf"] = mime::OTF;
    mimeTypes_["eot"] = mime::EOT;
    
    mimeTypes_["pdf"] = mime::PDF;
    mimeTypes_["zip"] = mime::ZIP;
    mimeTypes_["gz"] = mime::GZIP;
    
    mimeTypes_["mp3"] = mime::MP3;
    mimeTypes_["wav"] = mime::WAV;
    mimeTypes_["mp4"] = mime::MP4;
    mimeTypes_["webm"] = mime::WEBM;
}

bool StaticHandler::init(const String& staticDir, const String& urlPath) {
    staticDir_ = staticDir;
    urlPath_ = urlPath;
    LOG_INFO("Static", "Static handler initialized: " + staticDir + " -> " + urlPath);
    return true;
}

Response StaticHandler::serve(const String& path, Request& request) {
    xSemaphoreTake(mutex_, portMAX_DELAY);
    
    // Check cache first
    if (settings::STATIC_CACHE_ENABLED) {
        auto it = cache_.find(path);
        if (it != cache_.end()) {
            // Check if client cache is valid
            if (isClientCacheValid(request, it->second->etag)) {
                cacheHits_++;
                xSemaphoreGive(mutex_);
                
                Response response;
                response.setStatus(HttpStatus::NOT_MODIFIED);
                return response;
            }
            
            cacheHits_++;
            
            Response response;
            response.setStatus(HttpStatus::OK);
            response.setContentType(it->second->mimeType);
            response.setHeader("ETag", it->second->etag);
            response.setHeader("Cache-Control", "public, max-age=" + 
                              String(settings::CACHE_TTL_SECONDS));
            
            // Copy cached data to body
            response.body.reserve(it->second->size);
            for (size_t i = 0; i < it->second->size; i++) {
                response.body += (char)it->second->data[i];
            }
            
            xSemaphoreGive(mutex_);
            return response;
        }
    }
    
    cacheMisses_++;
    
    // Check if file exists
    if (!Files().exists(path)) {
        xSemaphoreGive(mutex_);
        return Response::error(HttpStatus::NOT_FOUND, "File not found: " + path);
    }
    
    // Get file info
    FileInfo info;
    if (!Files().getFileInfo(path, info)) {
        xSemaphoreGive(mutex_);
        return Response::error(HttpStatus::INTERNAL_SERVER_ERROR);
    }
    
    // Handle range requests
    String rangeHeader = request.getHeader("Range");
    if (!rangeHeader.isEmpty()) {
        xSemaphoreGive(mutex_);
        return handleRangeRequest(request, path, info.size);
    }
    
    // Get MIME type
    String mimeType = getMimeType(path);
    
    // Calculate ETag
    String etag = calculateEtag(path, info.size, info.modTime);
    
    // Check client cache
    if (isClientCacheValid(request, etag)) {
        xSemaphoreGive(mutex_);
        
        Response response;
        response.setStatus(HttpStatus::NOT_MODIFIED);
        return response;
    }
    
    // Load file
    std::unique_ptr<uint8_t[]> data;
    size_t size;
    
    if (!loadFile(path, data, size)) {
        xSemaphoreGive(mutex_);
        return Response::error(HttpStatus::INTERNAL_SERVER_ERROR, "Failed to read file");
    }
    
    // Cache if appropriate
    if (shouldCache(path, size)) {
        auto cached = std::make_unique<CachedFile>();
        cached->path = path;
        cached->mimeType = mimeType;
        cached->data = std::move(data);
        cached->size = size;
        cached->cachedAt = millis();
        cached->etag = etag;
        
        // Cleanup old entries if needed
        cleanupCache();
        
        // Make copy for response before moving to cache
        data = std::make_unique<uint8_t[]>(size);
        memcpy(data.get(), cached->data.get(), size);
        
        currentCacheSize_ += size;
        cache_[path] = std::move(cached);
    }
    
    xSemaphoreGive(mutex_);
    
    // Build response
    Response response;
    response.setStatus(HttpStatus::OK);
    response.setContentType(mimeType);
    response.setHeader("ETag", etag);
    response.setHeader("Cache-Control", "public, max-age=" + 
                      String(settings::CACHE_TTL_SECONDS));
    response.setHeader("Content-Length", String(size));
    
    // Copy data to body
    response.body.reserve(size);
    for (size_t i = 0; i < size; i++) {
        response.body += (char)data[i];
    }
    
    return response;
}

bool StaticHandler::exists(const String& path) {
    return Files().exists(path);
}

String StaticHandler::getMimeType(const String& path) {
    int dotPos = path.lastIndexOf('.');
    if (dotPos < 0) {
        return mime::BINARY;
    }
    
    String ext = path.substring(dotPos + 1);
    ext.toLowerCase();
    
    auto it = mimeTypes_.find(ext);
    if (it != mimeTypes_.end()) {
        return it->second;
    }
    
    return mime::BINARY;
}

bool StaticHandler::preload(const String& path) {
    xSemaphoreTake(mutex_, portMAX_DELAY);
    
    if (!Files().exists(path)) {
        xSemaphoreGive(mutex_);
        return false;
    }
    
    FileInfo info;
    Files().getFileInfo(path, info);
    
    std::unique_ptr<uint8_t[]> data;
    size_t size;
    
    if (!loadFile(path, data, size)) {
        xSemaphoreGive(mutex_);
        return false;
    }
    
    auto cached = std::make_unique<CachedFile>();
    cached->path = path;
    cached->mimeType = getMimeType(path);
    cached->data = std::move(data);
    cached->size = size;
    cached->cachedAt = millis();
    cached->etag = calculateEtag(path, size, info.modTime);
    
    currentCacheSize_ += size;
    cache_[path] = std::move(cached);
    
    xSemaphoreGive(mutex_);
    
    LOG_DEBUG("Static", "Preloaded: " + path + " (" + String(size) + " bytes)");
    return true;
}

void StaticHandler::clearCache() {
    xSemaphoreTake(mutex_, portMAX_DELAY);
    cache_.clear();
    currentCacheSize_ = 0;
    xSemaphoreGive(mutex_);
}

void StaticHandler::invalidate(const String& path) {
    xSemaphoreTake(mutex_, portMAX_DELAY);
    
    auto it = cache_.find(path);
    if (it != cache_.end()) {
        currentCacheSize_ -= it->second->size;
        cache_.erase(it);
    }
    
    xSemaphoreGive(mutex_);
}

void StaticHandler::getCacheStats(size_t& size, size_t& count,
                                  uint32_t& hits, uint32_t& misses) const {
    size = currentCacheSize_;
    count = cache_.size();
    hits = cacheHits_;
    misses = cacheMisses_;
}

void StaticHandler::registerMimeType(const String& extension, const String& mimeType) {
    String ext = extension;
    ext.toLowerCase();
    mimeTypes_[ext] = mimeType;
}

bool StaticHandler::loadFile(const String& path, std::unique_ptr<uint8_t[]>& data, size_t& size) {
    size = Files().getFileSize(path);
    if (size == 0) {
        return false;
    }
    
    // Allocate in PSRAM if available and file is large
    if (size > 4096 && psramFound()) {
        data.reset((uint8_t*)ps_malloc(size));
    } else {
        data = std::make_unique<uint8_t[]>(size);
    }
    
    if (!data) {
        return false;
    }
    
    return Files().readFileBinary(path, data.get(), size) > 0;
}

bool StaticHandler::shouldCache(const String& path, size_t size) {
    if (!settings::STATIC_CACHE_ENABLED) {
        return false;
    }
    
    if (size > settings::STATIC_CACHE_MAX_FILE_SIZE) {
        return false;
    }
    
    if (currentCacheSize_ + size > settings::STATIC_CACHE_MAX_TOTAL_SIZE) {
        return false;
    }
    
    return true;
}

String StaticHandler::calculateEtag(const String& path, size_t size, uint32_t modTime) {
    // Simple ETag based on path, size, and modification time
    char etag[32];
    snprintf(etag, sizeof(etag), "\"%08x%08x\"", (unsigned int)size, modTime);
    return String(etag);
}

bool StaticHandler::isClientCacheValid(const Request& request, const String& etag) {
    String ifNoneMatch = request.getHeader("If-None-Match");
    if (!ifNoneMatch.isEmpty() && ifNoneMatch == etag) {
        return true;
    }
    return false;
}

Response StaticHandler::handleRangeRequest(const Request& request, 
                                           const String& path, size_t size) {
    String rangeHeader = request.getHeader("Range");
    
    // Parse range header: "bytes=start-end"
    if (!rangeHeader.startsWith("bytes=")) {
        return Response::error(HttpStatus::BAD_REQUEST, "Invalid range");
    }
    
    String rangeSpec = rangeHeader.substring(6);
    int dashPos = rangeSpec.indexOf('-');
    
    size_t start = 0;
    size_t end = size - 1;
    
    if (dashPos == 0) {
        // "-500" means last 500 bytes
        size_t suffix = rangeSpec.substring(1).toInt();
        start = size - suffix;
    } else if (dashPos == (int)rangeSpec.length() - 1) {
        // "500-" means from byte 500 to end
        start = rangeSpec.substring(0, dashPos).toInt();
    } else {
        start = rangeSpec.substring(0, dashPos).toInt();
        end = rangeSpec.substring(dashPos + 1).toInt();
    }
    
    if (start >= size || end >= size || start > end) {
        Response response;
        response.setStatus(HttpStatus::BAD_REQUEST);
        response.setHeader("Content-Range", "bytes */" + String(size));
        return response;
    }
    
    size_t rangeSize = end - start + 1;
    
    // Read the range
    std::unique_ptr<uint8_t[]> data;
    if (rangeSize > 4096 && psramFound()) {
        data.reset((uint8_t*)ps_malloc(rangeSize));
    } else {
        data = std::make_unique<uint8_t[]>(rangeSize);
    }
    
    if (!data) {
        return Response::error(HttpStatus::INTERNAL_SERVER_ERROR, "Memory allocation failed");
    }
    
    // Read file range (simplified - in reality would need file seek)
    std::unique_ptr<uint8_t[]> fullData;
    size_t fullSize;
    if (!loadFile(path, fullData, fullSize)) {
        return Response::error(HttpStatus::INTERNAL_SERVER_ERROR, "Failed to read file");
    }
    
    memcpy(data.get(), fullData.get() + start, rangeSize);
    
    // Build response
    Response response;
    response.status = HttpStatus::OK; // Would be 206 Partial Content
    response.setContentType(getMimeType(path));
    response.setHeader("Content-Range", 
                      "bytes " + String(start) + "-" + String(end) + "/" + String(size));
    response.setHeader("Accept-Ranges", "bytes");
    response.setHeader("Content-Length", String(rangeSize));
    
    response.body.reserve(rangeSize);
    for (size_t i = 0; i < rangeSize; i++) {
        response.body += (char)data[i];
    }
    
    return response;
}

void StaticHandler::cleanupCache() {
    // Remove oldest entries if cache is full
    while (currentCacheSize_ > settings::STATIC_CACHE_MAX_TOTAL_SIZE * 0.9 && !cache_.empty()) {
        // Find oldest entry
        String oldestKey;
        uint32_t oldestTime = UINT32_MAX;
        
        for (const auto& entry : cache_) {
            if (entry.second->cachedAt < oldestTime) {
                oldestTime = entry.second->cachedAt;
                oldestKey = entry.first;
            }
        }
        
        if (!oldestKey.isEmpty()) {
            currentCacheSize_ -= cache_[oldestKey]->size;
            cache_.erase(oldestKey);
        }
    }
}

} // namespace espweb
