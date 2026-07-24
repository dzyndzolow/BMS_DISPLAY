/**
 * @file fileio.hpp
 * @brief ESP32 Web Framework - File I/O Module
 * 
 * Handles file operations:
 * - File upload (multipart/form-data)
 * - File download with resume support
 * - SD card and SPIFFS integration
 * - Stream-based file handling
 * 
 * @author ESP32 Web Framework
 * @version 1.0.0
 */

#ifndef ESP_WEB_FRAMEWORK_FILEIO_HPP
#define ESP_WEB_FRAMEWORK_FILEIO_HPP

#include <Arduino.h>
#include <WiFi.h>
#include <FS.h>
#include <SD.h>
#include <SPIFFS.h>
#include <vector>
#include <functional>

#include "http.hpp"
#include "settings.h"

namespace espweb {

/**
 * @brief File system type
 */
enum class FileSystemType {
    SPIFFS,
    SD_CARD,
    AUTO        ///< Auto-detect based on path prefix
};

/**
 * @brief File info structure
 */
struct FileInfo {
    String path;
    String name;
    size_t size;
    bool isDirectory;
    uint32_t modTime;
};

/**
 * @brief Upload progress callback
 */
using UploadProgressCallback = std::function<void(const String& filename, size_t current, size_t total)>;

/**
 * @brief Upload complete callback
 */
using UploadCompleteCallback = std::function<void(const UploadedFile& file, bool success)>;

/**
 * @brief File I/O handler - Singleton
 * 
 * Manages file operations across SPIFFS and SD card.
 */
class FileIO {
public:
    /**
     * @brief Get singleton instance
     */
    static FileIO& getInstance();
    
    // Delete copy constructor and assignment
    FileIO(const FileIO&) = delete;
    FileIO& operator=(const FileIO&) = delete;
    
    /**
     * @brief Initialize file systems
     * @param initSD Initialize SD card
     * @param initSPIFFS Initialize SPIFFS
     * @return true if at least one filesystem initialized
     */
    bool init(bool initSD = true, bool initSPIFFS = true);
    
    /**
     * @brief Check if SD card is available
     */
    bool hasSD() const { return sdAvailable_; }
    
    /**
     * @brief Check if SPIFFS is available
     */
    bool hasSPIFFS() const { return spiffsAvailable_; }
    
    //==========================================================================
    // File Operations
    //==========================================================================
    
    /**
     * @brief Read file content
     * @param path File path
     * @return File content or empty string
     */
    String readFile(const String& path);
    
    /**
     * @brief Read file as binary
     * @param path File path
     * @param buffer Output buffer
     * @param maxSize Maximum size to read
     * @return Bytes read or -1 on error
     */
    int readFileBinary(const String& path, uint8_t* buffer, size_t maxSize);
    
    /**
     * @brief Write string to file
     * @param path File path
     * @param content File content
     * @param append Append mode
     * @return true if successful
     */
    bool writeFile(const String& path, const String& content, bool append = false);
    
    /**
     * @brief Write binary to file
     * @param path File path
     * @param data Data buffer
     * @param size Data size
     * @param append Append mode
     * @return true if successful
     */
    bool writeFileBinary(const String& path, const uint8_t* data, size_t size, bool append = false);
    
    /**
     * @brief Delete file
     * @param path File path
     * @return true if successful
     */
    bool deleteFile(const String& path);
    
    /**
     * @brief Rename/move file
     * @param oldPath Old path
     * @param newPath New path
     * @return true if successful
     */
    bool renameFile(const String& oldPath, const String& newPath);
    
    /**
     * @brief Check if file exists
     * @param path File path
     * @return true if exists
     */
    bool exists(const String& path);
    
    /**
     * @brief Get file size
     * @param path File path
     * @return File size or 0 if not found
     */
    size_t getFileSize(const String& path);
    
    /**
     * @brief Get file info
     * @param path File path
     * @param info Output file info
     * @return true if successful
     */
    bool getFileInfo(const String& path, FileInfo& info);
    
    //==========================================================================
    // Directory Operations
    //==========================================================================
    
    /**
     * @brief Create directory
     * @param path Directory path
     * @return true if successful
     */
    bool createDir(const String& path);
    
    /**
     * @brief Delete directory
     * @param path Directory path
     * @param recursive Delete recursively
     * @return true if successful
     */
    bool deleteDir(const String& path, bool recursive = false);
    
    /**
     * @brief List directory contents
     * @param path Directory path
     * @return Vector of file info
     */
    std::vector<FileInfo> listDir(const String& path);
    
    //==========================================================================
    // Upload Handling
    //==========================================================================
    
    /**
     * @brief Parse multipart upload from request
     * @param request Request object
     * @param uploadDir Destination directory
     * @param progressCb Progress callback
     * @return Vector of uploaded files
     */
    std::vector<UploadedFile> parseUpload(Request& request, 
                                           const String& uploadDir = "/sd/uploads",
                                           UploadProgressCallback progressCb = nullptr);
    
    /**
     * @brief Set upload progress callback
     * @param callback Callback function
     */
    void setUploadProgressCallback(UploadProgressCallback callback) {
        uploadProgressCb_ = callback;
    }
    
    /**
     * @brief Set upload complete callback
     * @param callback Callback function
     */
    void setUploadCompleteCallback(UploadCompleteCallback callback) {
        uploadCompleteCb_ = callback;
    }
    
    //==========================================================================
    // Download Handling
    //==========================================================================
    
    /**
     * @brief Create download response
     * @param path File path
     * @param filename Download filename (optional)
     * @return Response object
     */
    Response createDownloadResponse(const String& path, const String& filename = "");
    
    /**
     * @brief Stream file to client
     * @param client WiFi client
     * @param path File path
     * @param rangeStart Range start (for resume)
     * @param rangeEnd Range end (for resume)
     * @return Bytes sent
     */
    size_t streamFile(WiFiClient& client, const String& path, 
                      size_t rangeStart = 0, size_t rangeEnd = 0);
    
    //==========================================================================
    // Utilities
    //==========================================================================
    
    /**
     * @brief Get filesystem for path
     * @param path File path
     * @return Pointer to FS object
     */
    fs::FS* getFS(const String& path);
    
    /**
     * @brief Convert path to filesystem path
     * @param path Input path (may include sd:// or spiffs://)
     * @param fsPath Output filesystem path
     * @return FileSystemType
     */
    FileSystemType resolvePath(const String& path, String& fsPath);
    
    /**
     * @brief Get free space
     * @param fsType Filesystem type
     * @return Free bytes
     */
    size_t getFreeSpace(FileSystemType fsType = FileSystemType::SD_CARD);
    
    /**
     * @brief Get total space
     * @param fsType Filesystem type
     * @return Total bytes
     */
    size_t getTotalSpace(FileSystemType fsType = FileSystemType::SD_CARD);
    
private:
    FileIO();
    ~FileIO() = default;
    
    /**
     * @brief Parse multipart boundary
     * @param contentType Content-Type header
     * @return Boundary string
     */
    String parseBoundary(const String& contentType);
    
    /**
     * @brief Parse multipart header
     * @param header Header string
     * @param name Output field name
     * @param filename Output filename
     * @param contentType Output content type
     */
    void parsePartHeader(const String& header, String& name, 
                         String& filename, String& contentType);
    
    bool sdAvailable_ = false;
    bool spiffsAvailable_ = false;
    
    UploadProgressCallback uploadProgressCb_;
    UploadCompleteCallback uploadCompleteCb_;
    
    SemaphoreHandle_t mutex_;
};

/**
 * @brief Convenience function to get file I/O handler
 */
inline FileIO& Files() {
    return FileIO::getInstance();
}

} // namespace espweb

#endif // ESP_WEB_FRAMEWORK_FILEIO_HPP
