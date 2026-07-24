/**
 * @file orm.cpp
 * @brief ESP32 Web Framework - ORM Implementation
 * 
 * @author ESP32 Web Framework
 * @version 1.0.0
 */

#include "orm.hpp"
#include "logger.hpp"

namespace espweb {

//==============================================================================
// Database Implementation
//==============================================================================

Database& Database::getInstance() {
    static Database instance;
    return instance;
}

Database::Database() {
    mutex_ = xSemaphoreCreateMutex();
}

bool Database::init(const String& dataDir) {
    dataDir_ = dataDir;
    
    // Ensure directory exists
    Files().createDir(dataDir_);
    
    LOG_INFO("ORM", "Database initialized at: " + dataDir_);
    return true;
}

String Database::getFilePath(const String& modelName) {
    return dataDir_ + "/" + modelName + ".json";
}

bool Database::loadData(const String& modelName, JsonDocument& doc) {
    String path = getFilePath(modelName);
    String content = Files().readFile(path);
    
    if (content.isEmpty()) {
        return false;
    }
    
    DeserializationError error = deserializeJson(doc, content);
    return error == DeserializationError::Ok;
}

bool Database::saveData(const String& modelName, const JsonDocument& doc) {
    String path = getFilePath(modelName);
    String content;
    serializeJson(doc, content);
    
    return Files().writeFile(path, content);
}

uint32_t Database::getNextId(const String& modelName) {
    auto it = nextIds_.find(modelName);
    if (it == nextIds_.end()) {
        // Load from file to find max ID
        JsonDocument doc;
        if (loadData(modelName, doc)) {
            JsonArray arr = doc.as<JsonArray>();
            uint32_t maxId = 0;
            for (JsonVariant item : arr) {
                uint32_t id = item["id"] | 0;
                if (id > maxId) maxId = id;
            }
            nextIds_[modelName] = maxId + 1;
        } else {
            nextIds_[modelName] = 1;
        }
    }
    
    return nextIds_[modelName]++;
}

void Database::sync() {
    // Force write all pending changes
    dirty_ = false;
}

void Database::clear() {
    xSemaphoreTake(mutex_, portMAX_DELAY);
    nextIds_.clear();
    xSemaphoreGive(mutex_);
}

//==============================================================================
// Settings Implementation
//==============================================================================

Settings& Settings::getInstance() {
    static Settings instance;
    return instance;
}

bool Settings::init(const String& namespace_) {
    if (initialized_) {
        prefs_.end();
    }
    
    bool result = prefs_.begin(namespace_.c_str(), false);
    initialized_ = result;
    
    LOG_INFO("Settings", "NVS Settings initialized: " + namespace_);
    return result;
}

String Settings::getString(const String& key, const String& defaultValue) {
    if (!initialized_) return defaultValue;
    return prefs_.getString(key.c_str(), defaultValue);
}

bool Settings::setString(const String& key, const String& value) {
    if (!initialized_) return false;
    return prefs_.putString(key.c_str(), value) > 0;
}

int Settings::getInt(const String& key, int defaultValue) {
    if (!initialized_) return defaultValue;
    return prefs_.getInt(key.c_str(), defaultValue);
}

bool Settings::setInt(const String& key, int value) {
    if (!initialized_) return false;
    return prefs_.putInt(key.c_str(), value) > 0;
}

float Settings::getFloat(const String& key, float defaultValue) {
    if (!initialized_) return defaultValue;
    return prefs_.getFloat(key.c_str(), defaultValue);
}

bool Settings::setFloat(const String& key, float value) {
    if (!initialized_) return false;
    return prefs_.putFloat(key.c_str(), value) > 0;
}

bool Settings::getBool(const String& key, bool defaultValue) {
    if (!initialized_) return defaultValue;
    return prefs_.getBool(key.c_str(), defaultValue);
}

bool Settings::setBool(const String& key, bool value) {
    if (!initialized_) return false;
    return prefs_.putBool(key.c_str(), value);
}

bool Settings::exists(const String& key) {
    if (!initialized_) return false;
    return prefs_.isKey(key.c_str());
}

bool Settings::remove(const String& key) {
    if (!initialized_) return false;
    return prefs_.remove(key.c_str());
}

bool Settings::clear() {
    if (!initialized_) return false;
    return prefs_.clear();
}

} // namespace espweb
