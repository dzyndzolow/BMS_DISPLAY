/**
 * @file cache.hpp
 * @brief ESP32 Web Framework - Cache System
 * 
 * Caching system for RAM and PSRAM:
 * - Key-value cache
 * - Template cache
 * - Static file cache
 * - TTL support
 * - LRU eviction
 * 
 * @author ESP32 Web Framework
 * @version 1.0.0
 */

#ifndef ESP_WEB_FRAMEWORK_CACHE_HPP
#define ESP_WEB_FRAMEWORK_CACHE_HPP

#include <Arduino.h>
#include <map>
#include <list>
#include <memory>
#include <functional>

#include "settings.h"

namespace espweb {

/**
 * @brief Cache entry metadata
 */
struct CacheEntry {
    String key;
    size_t size;
    uint32_t createdAt;
    uint32_t expiresAt;
    uint32_t lastAccessedAt;
    uint32_t accessCount;
};

/**
 * @brief Cache statistics
 */
struct CacheStats {
    size_t itemCount;
    size_t totalSize;
    size_t maxSize;
    uint32_t hits;
    uint32_t misses;
    float hitRate;
};

/**
 * @brief Generic cache template
 * 
 * LRU cache with TTL support, stored in PSRAM when available.
 */
template<typename T>
class Cache {
public:
    /**
     * @brief Constructor
     * @param maxSize Maximum cache size in bytes
     * @param defaultTtl Default TTL in seconds (0 = no expiry)
     */
    Cache(size_t maxSize = 1024 * 1024, uint32_t defaultTtl = 3600);
    
    ~Cache() = default;
    
    /**
     * @brief Get item from cache
     * @param key Cache key
     * @return Pointer to item or nullptr
     */
    T* get(const String& key);
    
    /**
     * @brief Set item in cache
     * @param key Cache key
     * @param value Item value
     * @param ttl TTL in seconds (0 = use default)
     * @return true if stored
     */
    bool set(const String& key, T value, uint32_t ttl = 0);
    
    /**
     * @brief Check if key exists
     * @param key Cache key
     * @return true if exists and not expired
     */
    bool has(const String& key);
    
    /**
     * @brief Remove item from cache
     * @param key Cache key
     * @return true if removed
     */
    bool remove(const String& key);
    
    /**
     * @brief Clear all items
     */
    void clear();
    
    /**
     * @brief Remove expired items
     * @return Number of items removed
     */
    size_t cleanup();
    
    /**
     * @brief Get cache statistics
     */
    CacheStats getStats() const;
    
    /**
     * @brief Get all keys
     */
    std::vector<String> getKeys() const;
    
    /**
     * @brief Get entry metadata
     */
    CacheEntry* getEntry(const String& key);
    
private:
    /**
     * @brief Evict items until size is under limit
     */
    void evict(size_t requiredSpace);
    
    /**
     * @brief Move item to front (most recently used)
     */
    void touch(const String& key);
    
    /**
     * @brief Estimate size of item
     */
    size_t estimateSize(const T& item);
    
    struct CacheItem {
        T value;
        CacheEntry entry;
    };
    
    std::map<String, CacheItem> items_;
    std::list<String> lruOrder_;
    
    size_t maxSize_;
    size_t currentSize_ = 0;
    uint32_t defaultTtl_;
    
    uint32_t hits_ = 0;
    uint32_t misses_ = 0;
    
    SemaphoreHandle_t mutex_;
};

/**
 * @brief String cache specialization
 */
using StringCache = Cache<String>;

/**
 * @brief Binary cache for raw data
 */
class BinaryCache {
public:
    BinaryCache(size_t maxSize = 1024 * 1024, uint32_t defaultTtl = 3600);
    ~BinaryCache();
    
    /**
     * @brief Get binary data
     * @param key Cache key
     * @param size Output data size
     * @return Pointer to data or nullptr
     */
    const uint8_t* get(const String& key, size_t& size);
    
    /**
     * @brief Set binary data
     * @param key Cache key
     * @param data Data pointer
     * @param size Data size
     * @param ttl TTL in seconds
     * @return true if stored
     */
    bool set(const String& key, const uint8_t* data, size_t size, uint32_t ttl = 0);
    
    /**
     * @brief Check if key exists
     */
    bool has(const String& key);
    
    /**
     * @brief Remove item
     */
    bool remove(const String& key);
    
    /**
     * @brief Clear all items
     */
    void clear();
    
    /**
     * @brief Remove expired items
     */
    size_t cleanup();
    
    /**
     * @brief Get statistics
     */
    CacheStats getStats() const;
    
private:
    struct BinaryItem {
        std::unique_ptr<uint8_t[]> data;
        size_t size;
        CacheEntry entry;
    };
    
    void evict(size_t requiredSpace);
    
    std::map<String, BinaryItem> items_;
    std::list<String> lruOrder_;
    
    size_t maxSize_;
    size_t currentSize_ = 0;
    uint32_t defaultTtl_;
    
    uint32_t hits_ = 0;
    uint32_t misses_ = 0;
    
    SemaphoreHandle_t mutex_;
};

/**
 * @brief Global cache manager - Singleton
 * 
 * Manages multiple cache instances.
 */
class CacheManager {
public:
    /**
     * @brief Get singleton instance
     */
    static CacheManager& getInstance();
    
    // Delete copy constructor and assignment
    CacheManager(const CacheManager&) = delete;
    CacheManager& operator=(const CacheManager&) = delete;
    
    /**
     * @brief Initialize cache manager
     */
    bool init();
    
    /**
     * @brief Get string cache
     */
    StringCache& getStringCache() { return stringCache_; }
    
    /**
     * @brief Get binary cache
     */
    BinaryCache& getBinaryCache() { return binaryCache_; }
    
    /**
     * @brief Get response cache (for API responses)
     */
    StringCache& getResponseCache() { return responseCache_; }
    
    /**
     * @brief Clear all caches
     */
    void clearAll();
    
    /**
     * @brief Cleanup all caches
     */
    size_t cleanupAll();
    
    /**
     * @brief Get total cache size
     */
    size_t getTotalSize();
    
    /**
     * @brief Get combined statistics
     */
    CacheStats getCombinedStats();
    
private:
    CacheManager();
    ~CacheManager() = default;
    
    StringCache stringCache_;
    BinaryCache binaryCache_;
    StringCache responseCache_;
};

/**
 * @brief Convenience function to get cache manager
 */
inline CacheManager& Caches() {
    return CacheManager::getInstance();
}

//==============================================================================
// Cache Implementation
//==============================================================================

template<typename T>
Cache<T>::Cache(size_t maxSize, uint32_t defaultTtl)
    : maxSize_(maxSize), defaultTtl_(defaultTtl) {
    mutex_ = xSemaphoreCreateMutex();
}

template<typename T>
T* Cache<T>::get(const String& key) {
    xSemaphoreTake(mutex_, portMAX_DELAY);
    
    auto it = items_.find(key);
    if (it == items_.end()) {
        misses_++;
        xSemaphoreGive(mutex_);
        return nullptr;
    }
    
    // Check expiry
    if (it->second.entry.expiresAt > 0 && 
        millis() > it->second.entry.expiresAt) {
        items_.erase(it);
        lruOrder_.remove(key);
        misses_++;
        xSemaphoreGive(mutex_);
        return nullptr;
    }
    
    // Update access info
    it->second.entry.lastAccessedAt = millis();
    it->second.entry.accessCount++;
    hits_++;
    
    // Move to front of LRU
    lruOrder_.remove(key);
    lruOrder_.push_front(key);
    
    T* result = &(it->second.value);
    xSemaphoreGive(mutex_);
    return result;
}

template<typename T>
bool Cache<T>::set(const String& key, T value, uint32_t ttl) {
    xSemaphoreTake(mutex_, portMAX_DELAY);
    
    size_t itemSize = estimateSize(value);
    
    // Remove existing if present
    auto existing = items_.find(key);
    if (existing != items_.end()) {
        currentSize_ -= existing->second.entry.size;
        items_.erase(existing);
        lruOrder_.remove(key);
    }
    
    // Evict if necessary
    if (currentSize_ + itemSize > maxSize_) {
        evict(itemSize);
    }
    
    // Create entry
    CacheItem item;
    item.value = std::move(value);
    item.entry.key = key;
    item.entry.size = itemSize;
    item.entry.createdAt = millis();
    item.entry.lastAccessedAt = millis();
    item.entry.accessCount = 0;
    
    uint32_t actualTtl = (ttl > 0) ? ttl : defaultTtl_;
    item.entry.expiresAt = (actualTtl > 0) ? (millis() + actualTtl * 1000) : 0;
    
    items_[key] = std::move(item);
    lruOrder_.push_front(key);
    currentSize_ += itemSize;
    
    xSemaphoreGive(mutex_);
    return true;
}

template<typename T>
bool Cache<T>::has(const String& key) {
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

template<typename T>
bool Cache<T>::remove(const String& key) {
    xSemaphoreTake(mutex_, portMAX_DELAY);
    
    auto it = items_.find(key);
    if (it == items_.end()) {
        xSemaphoreGive(mutex_);
        return false;
    }
    
    currentSize_ -= it->second.entry.size;
    items_.erase(it);
    lruOrder_.remove(key);
    
    xSemaphoreGive(mutex_);
    return true;
}

template<typename T>
void Cache<T>::clear() {
    xSemaphoreTake(mutex_, portMAX_DELAY);
    items_.clear();
    lruOrder_.clear();
    currentSize_ = 0;
    xSemaphoreGive(mutex_);
}

template<typename T>
size_t Cache<T>::cleanup() {
    xSemaphoreTake(mutex_, portMAX_DELAY);
    
    size_t removed = 0;
    uint32_t now = millis();
    
    for (auto it = items_.begin(); it != items_.end();) {
        if (it->second.entry.expiresAt > 0 && now > it->second.entry.expiresAt) {
            currentSize_ -= it->second.entry.size;
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

template<typename T>
CacheStats Cache<T>::getStats() const {
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

template<typename T>
std::vector<String> Cache<T>::getKeys() const {
    std::vector<String> keys;
    for (const auto& item : items_) {
        keys.push_back(item.first);
    }
    return keys;
}

template<typename T>
void Cache<T>::evict(size_t requiredSpace) {
    while (currentSize_ + requiredSpace > maxSize_ && !lruOrder_.empty()) {
        String lruKey = lruOrder_.back();
        lruOrder_.pop_back();
        
        auto it = items_.find(lruKey);
        if (it != items_.end()) {
            currentSize_ -= it->second.entry.size;
            items_.erase(it);
        }
    }
}

template<typename T>
size_t Cache<T>::estimateSize(const T& item) {
    // Default implementation for String
    return sizeof(T);
}

// Specialization for String
template<>
inline size_t Cache<String>::estimateSize(const String& item) {
    return item.length() + sizeof(CacheEntry) + 16; // overhead
}

} // namespace espweb

#endif // ESP_WEB_FRAMEWORK_CACHE_HPP
