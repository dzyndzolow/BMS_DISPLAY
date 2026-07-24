/**
 * @file orm.hpp
 * @brief ESP32 Web Framework - Simple ORM
 * 
 * JSON-based ORM for data persistence:
 * - Models as classes
 * - JSON file storage
 * - NVS for settings
 * - CRUD operations
 * 
 * @author ESP32 Web Framework
 * @version 1.0.0
 */

#ifndef ESP_WEB_FRAMEWORK_ORM_HPP
#define ESP_WEB_FRAMEWORK_ORM_HPP

#include <Arduino.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include <vector>
#include <map>
#include <functional>
#include <memory>

#include "fileio.hpp"
#include "settings.h"

namespace espweb {

/**
 * @brief Field types for model definition
 */
enum class FieldType {
    STRING,
    INT,
    FLOAT,
    BOOL,
    DATETIME,
    JSON
};

/**
 * @brief Model field definition
 */
struct Field {
    String name;
    FieldType type;
    bool required = false;
    bool unique = false;
    String defaultValue;
};

/**
 * @brief Query filter
 */
struct QueryFilter {
    String field;
    String op;      // eq, ne, gt, lt, gte, lte, contains, startswith, endswith
    String value;
};

/**
 * @brief Base Model class
 * 
 * Inherit from this to create data models.
 */
class Model {
public:
    Model() : id_(0), created_(0), modified_(0) {}
    virtual ~Model() = default;
    
    /**
     * @brief Get model ID
     */
    uint32_t getId() const { return id_; }
    
    /**
     * @brief Set model ID
     */
    void setId(uint32_t id) { id_ = id; }
    
    /**
     * @brief Get creation timestamp
     */
    uint32_t getCreated() const { return created_; }
    
    /**
     * @brief Get modification timestamp
     */
    uint32_t getModified() const { return modified_; }
    
    /**
     * @brief Get model name (table name)
     */
    virtual const char* getModelName() const = 0;
    
    /**
     * @brief Get field definitions
     */
    virtual std::vector<Field> getFields() const = 0;
    
    /**
     * @brief Serialize to JSON
     */
    virtual void toJson(JsonObject& obj) const = 0;
    
    /**
     * @brief Deserialize from JSON
     */
    virtual void fromJson(const JsonObject& obj) = 0;
    
    /**
     * @brief Validate model data
     * @param errors Output error messages
     * @return true if valid
     */
    virtual bool validate(std::map<String, String>& errors) const { return true; }
    
protected:
    uint32_t id_;
    uint32_t created_;
    uint32_t modified_;
    
    friend class Database;
};

/**
 * @brief Query builder for filtering
 */
template<typename T>
class Query {
public:
    Query(class Database& db) : db_(db) {}
    
    /**
     * @brief Add filter condition
     */
    Query& filter(const String& field, const String& op, const String& value);
    
    /**
     * @brief Filter by exact match
     */
    Query& where(const String& field, const String& value);
    
    /**
     * @brief Order by field
     */
    Query& orderBy(const String& field, bool desc = false);
    
    /**
     * @brief Limit results
     */
    Query& limit(size_t count);
    
    /**
     * @brief Offset results
     */
    Query& offset(size_t count);
    
    /**
     * @brief Get all matching records
     */
    std::vector<std::unique_ptr<T>> all();
    
    /**
     * @brief Get first matching record
     */
    std::unique_ptr<T> first();
    
    /**
     * @brief Get record count
     */
    size_t count();
    
    /**
     * @brief Check if any records match
     */
    bool exists();
    
    /**
     * @brief Delete matching records
     */
    size_t remove();
    
private:
    Database& db_;
    std::vector<QueryFilter> filters_;
    String orderField_;
    bool orderDesc_ = false;
    size_t limitCount_ = 0;
    size_t offsetCount_ = 0;
};

/**
 * @brief Database manager - Singleton
 * 
 * Manages JSON-based data storage.
 */
class Database {
public:
    /**
     * @brief Get singleton instance
     */
    static Database& getInstance();
    
    // Delete copy constructor and assignment
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;
    
    /**
     * @brief Initialize database
     * @param dataDir Data directory
     * @return true if successful
     */
    bool init(const String& dataDir = ORM_DATA_DIR);
    
    //==========================================================================
    // CRUD Operations
    //==========================================================================
    
    /**
     * @brief Save model (insert or update)
     * @param model Model to save
     * @return true if successful
     */
    template<typename T>
    bool save(T& model);
    
    /**
     * @brief Get model by ID
     * @param id Model ID
     * @return Unique pointer to model or nullptr
     */
    template<typename T>
    std::unique_ptr<T> get(uint32_t id);
    
    /**
     * @brief Get all models
     * @return Vector of models
     */
    template<typename T>
    std::vector<std::unique_ptr<T>> all();
    
    /**
     * @brief Delete model
     * @param model Model to delete
     * @return true if successful
     */
    template<typename T>
    bool remove(const T& model);
    
    /**
     * @brief Delete by ID
     * @param id Model ID
     * @return true if successful
     */
    template<typename T>
    bool remove(uint32_t id);
    
    /**
     * @brief Create query builder
     */
    template<typename T>
    Query<T> query();
    
    //==========================================================================
    // Bulk Operations
    //==========================================================================
    
    /**
     * @brief Save multiple models
     */
    template<typename T>
    size_t saveAll(std::vector<T>& models);
    
    /**
     * @brief Delete all models
     */
    template<typename T>
    size_t removeAll();
    
    //==========================================================================
    // Utilities
    //==========================================================================
    
    /**
     * @brief Get next ID for model
     */
    uint32_t getNextId(const String& modelName);
    
    /**
     * @brief Sync data to disk
     */
    void sync();
    
    /**
     * @brief Clear all data
     */
    void clear();
    
    /**
     * @brief Export to JSON
     */
    template<typename T>
    String exportJson();
    
    /**
     * @brief Import from JSON
     */
    template<typename T>
    size_t importJson(const String& json);
    
private:
    Database();
    ~Database() = default;
    
    /**
     * @brief Get data file path for model
     */
    String getFilePath(const String& modelName);
    
    /**
     * @brief Load data from file
     */
    bool loadData(const String& modelName, JsonDocument& doc);
    
    /**
     * @brief Save data to file
     */
    bool saveData(const String& modelName, const JsonDocument& doc);
    
    String dataDir_;
    std::map<String, uint32_t> nextIds_;
    bool dirty_ = false;
    
    SemaphoreHandle_t mutex_;
};

/**
 * @brief NVS Settings storage
 * 
 * For simple key-value settings using ESP32 NVS.
 */
class Settings {
public:
    /**
     * @brief Get singleton instance
     */
    static Settings& getInstance();
    
    // Delete copy constructor and assignment
    Settings(const Settings&) = delete;
    Settings& operator=(const Settings&) = delete;
    
    /**
     * @brief Initialize settings
     * @param namespace_ NVS namespace
     * @return true if successful
     */
    bool init(const String& namespace_ = "espweb");
    
    /**
     * @brief Get string value
     */
    String getString(const String& key, const String& defaultValue = "");
    
    /**
     * @brief Set string value
     */
    bool setString(const String& key, const String& value);
    
    /**
     * @brief Get integer value
     */
    int getInt(const String& key, int defaultValue = 0);
    
    /**
     * @brief Set integer value
     */
    bool setInt(const String& key, int value);
    
    /**
     * @brief Get float value
     */
    float getFloat(const String& key, float defaultValue = 0.0f);
    
    /**
     * @brief Set float value
     */
    bool setFloat(const String& key, float value);
    
    /**
     * @brief Get boolean value
     */
    bool getBool(const String& key, bool defaultValue = false);
    
    /**
     * @brief Set boolean value
     */
    bool setBool(const String& key, bool value);
    
    /**
     * @brief Check if key exists
     */
    bool exists(const String& key);
    
    /**
     * @brief Remove key
     */
    bool remove(const String& key);
    
    /**
     * @brief Clear all settings
     */
    bool clear();
    
private:
    Settings() = default;
    ~Settings() = default;
    
    Preferences prefs_;
    bool initialized_ = false;
};

/**
 * @brief Convenience function to get database
 */
inline Database& DB() {
    return Database::getInstance();
}

/**
 * @brief Convenience function to get settings
 */
inline Settings& Config() {
    return Settings::getInstance();
}

} // namespace espweb

#endif // ESP_WEB_FRAMEWORK_ORM_HPP
