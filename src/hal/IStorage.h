/**
 * @file IStorage.h
 * @brief Storage Hardware Abstraction Layer Interface
 * 
 * Defines the contract for storage implementations (SD, SPIFFS, LittleFS).
 * Unified file operations regardless of storage medium.
 * 
 * @version 1.0.0 (MASTER_PLAN Phase 1)
 */

#ifndef HAL_ISTORAGE_H
#define HAL_ISTORAGE_H

#include <cstdint>
#include <cstddef>

namespace HAL {

/**
 * @brief File open modes
 */
enum class FileMode : uint8_t {
    READ,
    WRITE,
    APPEND,
    READ_WRITE
};

/**
 * @brief File type enumeration
 */
enum class FileType : uint8_t {
    REGULAR_FILE,
    DIRECTORY,
    UNKNOWN
};

/**
 * @brief File information structure
 */
struct FileInfo {
    char name[64];
    uint32_t size;
    FileType type;
    uint32_t lastModified;  /* Unix timestamp if available */
};

/**
 * @brief Storage statistics
 */
struct StorageStats {
    uint64_t totalBytes;
    uint64_t usedBytes;
    uint64_t freeBytes;
};

/**
 * @brief Abstract file handle
 */
class IFile {
public:
    virtual ~IFile() = default;
    
    virtual size_t read(uint8_t* buffer, size_t length) = 0;
    virtual size_t write(const uint8_t* buffer, size_t length) = 0;
    virtual size_t print(const char* str) = 0;
    virtual size_t println(const char* str) = 0;
    virtual bool seek(uint32_t position) = 0;
    virtual uint32_t position() const = 0;
    virtual uint32_t size() const = 0;
    virtual void close() = 0;
    virtual bool isOpen() const = 0;
    virtual void flush() = 0;
};

/**
 * @brief Storage interface - abstracts file system operations
 */
class IStorage {
public:
    virtual ~IStorage() = default;
    
    /*==== Lifecycle ====*/
    
    /**
     * @brief Initialize storage (mount filesystem)
     * @return true on success
     */
    virtual bool init() = 0;
    
    /**
     * @brief Deinitialize (unmount filesystem)
     */
    virtual void deinit() = 0;
    
    /**
     * @brief Check if storage is mounted/ready
     */
    virtual bool isReady() const = 0;
    
    /*==== File Operations ====*/
    
    /**
     * @brief Open file
     * @param path File path
     * @param mode Open mode
     * @return File handle or nullptr on failure
     */
    virtual IFile* open(const char* path, FileMode mode) = 0;
    
    /**
     * @brief Check if file exists
     */
    virtual bool exists(const char* path) const = 0;
    
    /**
     * @brief Delete file
     */
    virtual bool remove(const char* path) = 0;
    
    /**
     * @brief Rename/move file
     */
    virtual bool rename(const char* oldPath, const char* newPath) = 0;
    
    /**
     * @brief Get file info
     */
    virtual bool getInfo(const char* path, FileInfo& info) const = 0;
    
    /*==== Directory Operations ====*/
    
    /**
     * @brief Create directory
     */
    virtual bool mkdir(const char* path) = 0;
    
    /**
     * @brief Remove directory
     */
    virtual bool rmdir(const char* path) = 0;
    
    /**
     * @brief List directory contents
     * @param path Directory path
     * @param entries Output array of file info
     * @param maxEntries Maximum entries to return
     * @return Number of entries found
     */
    virtual size_t listDir(const char* path, FileInfo* entries, size_t maxEntries) = 0;
    
    /*==== Statistics ====*/
    
    /**
     * @brief Get storage statistics
     */
    virtual StorageStats getStats() const = 0;
    
    /**
     * @brief Get storage name/type
     */
    virtual const char* getName() const = 0;
};

} // namespace HAL

#endif /* HAL_ISTORAGE_H */
