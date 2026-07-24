/**
 * @file cache.cpp
 * @brief ESP32 Web Framework - Cache Implementation
 * 
 * @author ESP32 Web Framework
 * @version 1.0.0
 */

#include "cache.hpp"
#include "logger.hpp"

namespace espweb {

//==============================================================================
// BinaryCache Implementation
//==============================================================================

BinaryCache::BinaryCache(size_t maxSize, uint32_t defaultTtl)
    : maxSize_(maxSize), defaultTtl_(defaultTtl) {
    mutex_ = xSemaphoreCreateMutex();
}

BinaryCache::~BinaryCache() {
    if (mutex_) {
        vSemaphoreDelete(mutex_);
    }
}

const uint8_t* BinaryCache::get(const String& key, size_t& size) {
    xSemaphoreTake(mutex_, portMAX_DELAY);
    
    auto it = items_.find(key);
    if (it == items_.end()) {
        misses_++;
        xSemaphoreGive(mutex_);
        size = 0;
        return nullptr;
    }
    
    // Check expiry
    if (it->second.entry.expiresAt > 0 && 
        millis() > it->second.entry.expiresAt) {
        currentSize_ -= it->second.size;
        items_.erase(it);
        lruOrder_.remove(key);
        misses_++;
        xSemaphoreGive(mutex_);
        size = 0;
        return nullptr;
    }
    
    // Update access info
    it->second.entry.lastAccessedAt = millis();
    it->second.entry.accessCount++;
    hits_++;
    
    // Move to front of LRU
    lruOrder_.remove(key);
    lruOrder_.push_front(key);
    
    size = it->second.size;
    const uint8_t* result = it->second.data.get();
    
    xSemaphoreGive(mutex_);
    return result;
}

bool BinaryCache::set(const String& key, const uint8_t* data, size_t size, uint32_t ttl) {
    xSemaphoreTake(mutex_, portMAX_DELAY);
    
    // Remove existing if present
    auto existing = items_.find(key);
    if (existing != items_.end()) {
        currentSize_ -= existing->second.size;
        items_.erase(existing);
        lruOrder_.remove(key);
    }
    
    // Evict if necessary
    if (currentSize_ + size > maxSize_) {
        evict(size);
    }
    
    // Allocate in PSRAM if available and large
    BinaryItem item;
    if (size > 4096 && psramFound()) {
        item.data.reset((uint8_t*)ps_malloc(size));
    } else {
        item.data = std::make_unique<uint8_t[]>(size);
    }
    
    if (!item.data) {
        xSemaphoreGive(mutex_);
        return false;
    }
    
    memcpy(item.data.get(), data, size);
    item.size = size;
    item.entry.key = key;
    item.entry.size = size;
    item.entry.createdAt = millis();
    item.entry.lastAccessedAt = millis();
    item.entry.accessCount = 0;
    
    uint32_t actualTtl = (ttl > 0) ? ttl : defaultTtl_;
    item.entry.expiresAt = (actualTtl > 0) ? (millis() + actualTtl * 1000) : 0;
    
    items_[key] = std::move(item);
    lruOrder_.push_front(key);
    currentSize_ += size;
    
    xSemaphoreGive(mutex_);
    return true;
}

bool BinaryCache::has(const String& key) {
    xSemaphoreTake(mutex_, portMAX_DELAY);
    
    auto it = items_.find(key);
    if (it == items_.end()) {
        xSemaphoreGive(mutex_);
        return false;
    }
    
    // Check expiry
    if (it->second.entry.expiresAt > 0 && 
        millis() > it->second.entry.expiresAt) {
        xSemaphoreGive(mutex_);
        return false;
    }
    
    xSemaphoreGive(mutex_);
    return true;
}

bool BinaryCache::remove(const String& key) {
    xSemaphoreTake(mutex_, portMAX_DELAY);
    
    auto it = items_.find(key);
    if (it == items_.end()) {
        xSemaphoreGive(mutex_);
        return false;
    }
    
    currentSize_ -= it->second.size;
    items_.erase(it);
    lruOrder_.remove(key);
    
    xSemaphoreGive(mutex_);
    return true;
}

void BinaryCache::clear() {
    xSemaphoreTake(mutex_, portMAX_DELAY);
    items_.clear();
    lruOrder_.clear();
    currentSize_ = 0;
    xSemaphoreGive(mutex_);
}

size_t BinaryCache::cleanup() {
    xSemaphoreTake(mutex_, portMAX_DELAY);
    
    size_t removed = 0;
    uint32_t now = millis();
    
    for (auto it = items_.begin(); it != items_.end();) {
        if (it->second.entry.expiresAt > 0 && now > it->second.entry.expiresAt) {
            currentSize_ -= it->second.size;
            lruOrder_.remove(it->first);
            it = items_.erase(it);
            removed++;
        } else {
            ++it;
        }
    }
    
    xSemaphoreGive(mutex_);
    return removed;
}

CacheStats BinaryCache::getStats() const {
    CacheStats stats;
    stats.itemCount = items_.size();
    stats.totalSize = currentSize_;
    stats.maxSize = maxSize_;
    stats.hits = hits_;
    stats.misses = misses_;
    stats.hitRate = (hits_ + misses_ > 0) ? 
                    (float)hits_ / (hits_ + misses_) : 0.0f;
    return stats;
}

void BinaryCache::evict(size_t requiredSpace) {
    while (currentSize_ + requiredSpace > maxSize_ && !lruOrder_.empty()) {
        String lruKey = lruOrder_.back();
        lruOrder_.pop_back();
        
        auto it = items_.find(lruKey);
        if (it != items_.end()) {
            currentSize_ -= it->second.size;
            items_.erase(it);
        }
    }
}

//==============================================================================
// CacheManager Implementation
//==============================================================================

CacheManager& CacheManager::getInstance() {
    static CacheManager instance;
    return instance;
}

CacheManager::CacheManager()
    : stringCache_(settings::STRING_CACHE_SIZE, settings::CACHE_TTL_SECONDS),
      binaryCache_(settings::BINARY_CACHE_SIZE, settings::CACHE_TTL_SECONDS),
      responseCache_(settings::RESPONSE_CACHE_SIZE, settings::RESPONSE_CACHE_TTL) {
}

bool CacheManager::init() {
    LOG_INFO("Cache", "Cache manager initialized");
    LOG_INFO("Cache", "String cache: " + String(settings::STRING_CACHE_SIZE / 1024) + "KB");
    LOG_INFO("Cache", "Binary cache: " + String(settings::BINARY_CACHE_SIZE / 1024) + "KB");
    LOG_INFO("Cache", "Response cache: " + String(settings::RESPONSE_CACHE_SIZE / 1024) + "KB");
    return true;
}

void CacheManager::clearAll() {
    stringCache_.clear();
    binaryCache_.clear();
    responseCache_.clear();
}

size_t CacheManager::cleanupAll() {
    size_t total = 0;
    total += stringCache_.cleanup();
    total += binaryCache_.cleanup();
    total += responseCache_.cleanup();
    return total;
}

size_t CacheManager::getTotalSize() {
    return stringCache_.getStats().totalSize +
           binaryCache_.getStats().totalSize +
           responseCache_.getStats().totalSize;
}

CacheStats CacheManager::getCombinedStats() {
    CacheStats string = stringCache_.getStats();
    CacheStats binary = binaryCache_.getStats();
    CacheStats response = responseCache_.getStats();
    
    CacheStats combined;
    combined.itemCount = string.itemCount + binary.itemCount + response.itemCount;
    combined.totalSize = string.totalSize + binary.totalSize + response.totalSize;
    combined.maxSize = string.maxSize + binary.maxSize + response.maxSize;
    combined.hits = string.hits + binary.hits + response.hits;
    combined.misses = string.misses + binary.misses + response.misses;
    combined.hitRate = (combined.hits + combined.misses > 0) ?
                       (float)combined.hits / (combined.hits + combined.misses) : 0.0f;
    
    return combined;
}

} // namespace espweb
