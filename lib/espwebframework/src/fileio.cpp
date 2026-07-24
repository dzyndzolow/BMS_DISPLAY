/**
 * @file fileio.cpp
 * @brief ESP32 Web Framework - File I/O Implementation
 * 
 * @author ESP32 Web Framework
 * @version 1.0.0
 */

#include "fileio.hpp"
#include "logger.hpp"
#include <WiFi.h>

namespace espweb {

// Singleton instance
FileIO& FileIO::getInstance() {
    static FileIO instance;
    return instance;
}

FileIO::FileIO() {
    mutex_ = xSemaphoreCreateMutex();
}

bool FileIO::init(bool initSD, bool initSPIFFS) {
    LOG_INFO("FileIO", "Initializing file systems...");
    
    // Initialize SPIFFS
    if (initSPIFFS) {
        if (SPIFFS.begin(true)) {
            spiffsAvailable_ = true;
            LOG_INFO("FileIO", "SPIFFS initialized. Total: " + 
                    String(SPIFFS.totalBytes()) + ", Used: " + 
                    String(SPIFFS.usedBytes()));
        } else {
            LOG_WARN("FileIO", "SPIFFS initialization failed");
        }
    }
    
    // Initialize SD card
    if (initSD) {
        if (SD.begin()) {
            sdAvailable_ = true;
            uint64_t cardSize = SD.cardSize() / (1024 * 1024);
            LOG_INFO("FileIO", "SD Card initialized. Size: " + String((uint32_t)cardSize) + "MB");
        } else {
            LOG_WARN("FileIO", "SD Card initialization failed");
        }
    }
    
    return spiffsAvailable_ || sdAvailable_;
}

//==============================================================================
// File Operations
//==============================================================================

String FileIO::readFile(const String& path) {
    xSemaphoreTake(mutex_, portMAX_DELAY);
    
    String fsPath;
    fs::FS* fs = getFS(path);
    resolvePath(path, fsPath);
    
    if (!fs) {
        xSemaphoreGive(mutex_);
        return "";
    }
    
    File file = fs->open(fsPath, FILE_READ);
    if (!file) {
        xSemaphoreGive(mutex_);
        return "";
    }
    
    String content = file.readString();
    file.close();
    
    xSemaphoreGive(mutex_);
    return content;
}

int FileIO::readFileBinary(const String& path, uint8_t* buffer, size_t maxSize) {
    xSemaphoreTake(mutex_, portMAX_DELAY);
    
    String fsPath;
    fs::FS* fs = getFS(path);
    resolvePath(path, fsPath);
    
    if (!fs) {
        xSemaphoreGive(mutex_);
        return -1;
    }
    
    File file = fs->open(fsPath, FILE_READ);
    if (!file) {
        xSemaphoreGive(mutex_);
        return -1;
    }
    
    size_t bytesToRead = min(maxSize, file.size());
    size_t bytesRead = file.read(buffer, bytesToRead);
    file.close();
    
    xSemaphoreGive(mutex_);
    return bytesRead;
}

bool FileIO::writeFile(const String& path, const String& content, bool append) {
    xSemaphoreTake(mutex_, portMAX_DELAY);
    
    String fsPath;
    fs::FS* fs = getFS(path);
    resolvePath(path, fsPath);
    
    if (!fs) {
        xSemaphoreGive(mutex_);
        return false;
    }
    
    // Ensure directory exists
    int lastSlash = fsPath.lastIndexOf('/');
    if (lastSlash > 0) {
        String dir = fsPath.substring(0, lastSlash);
        fs->mkdir(dir);
    }
    
    File file = fs->open(fsPath, append ? FILE_APPEND : FILE_WRITE);
    if (!file) {
        xSemaphoreGive(mutex_);
        return false;
    }
    
    size_t written = file.print(content);
    file.close();
    
    xSemaphoreGive(mutex_);
    return written == content.length();
}

bool FileIO::writeFileBinary(const String& path, const uint8_t* data, size_t size, bool append) {
    xSemaphoreTake(mutex_, portMAX_DELAY);
    
    String fsPath;
    fs::FS* fs = getFS(path);
    resolvePath(path, fsPath);
    
    if (!fs) {
        xSemaphoreGive(mutex_);
        return false;
    }
    
    // Ensure directory exists
    int lastSlash = fsPath.lastIndexOf('/');
    if (lastSlash > 0) {
        String dir = fsPath.substring(0, lastSlash);
        fs->mkdir(dir);
    }
    
    File file = fs->open(fsPath, append ? FILE_APPEND : FILE_WRITE);
    if (!file) {
        xSemaphoreGive(mutex_);
        return false;
    }
    
    size_t written = file.write(data, size);
    file.close();
    
    xSemaphoreGive(mutex_);
    return written == size;
}

bool FileIO::deleteFile(const String& path) {
    xSemaphoreTake(mutex_, portMAX_DELAY);
    
    String fsPath;
    fs::FS* fs = getFS(path);
    resolvePath(path, fsPath);
    
    if (!fs) {
        xSemaphoreGive(mutex_);
        return false;
    }
    
    bool result = fs->remove(fsPath);
    
    xSemaphoreGive(mutex_);
    return result;
}

bool FileIO::renameFile(const String& oldPath, const String& newPath) {
    xSemaphoreTake(mutex_, portMAX_DELAY);
    
    String oldFsPath, newFsPath;
    fs::FS* fs = getFS(oldPath);
    resolvePath(oldPath, oldFsPath);
    resolvePath(newPath, newFsPath);
    
    if (!fs) {
        xSemaphoreGive(mutex_);
        return false;
    }
    
    bool result = fs->rename(oldFsPath, newFsPath);
    
    xSemaphoreGive(mutex_);
    return result;
}

bool FileIO::exists(const String& path) {
    xSemaphoreTake(mutex_, portMAX_DELAY);
    
    String fsPath;
    fs::FS* fs = getFS(path);
    resolvePath(path, fsPath);
    
    if (!fs) {
        xSemaphoreGive(mutex_);
        return false;
    }
    
    bool result = fs->exists(fsPath);
    
    xSemaphoreGive(mutex_);
    return result;
}

size_t FileIO::getFileSize(const String& path) {
    xSemaphoreTake(mutex_, portMAX_DELAY);
    
    String fsPath;
    fs::FS* fs = getFS(path);
    resolvePath(path, fsPath);
    
    if (!fs) {
        xSemaphoreGive(mutex_);
        return 0;
    }
    
    File file = fs->open(fsPath, FILE_READ);
    if (!file) {
        xSemaphoreGive(mutex_);
        return 0;
    }
    
    size_t size = file.size();
    file.close();
    
    xSemaphoreGive(mutex_);
    return size;
}

bool FileIO::getFileInfo(const String& path, FileInfo& info) {
    xSemaphoreTake(mutex_, portMAX_DELAY);
    
    String fsPath;
    fs::FS* fs = getFS(path);
    resolvePath(path, fsPath);
    
    if (!fs) {
        xSemaphoreGive(mutex_);
        return false;
    }
    
    File file = fs->open(fsPath, FILE_READ);
    if (!file) {
        xSemaphoreGive(mutex_);
        return false;
    }
    
    info.path = path;
    info.name = file.name();
    info.size = file.size();
    info.isDirectory = file.isDirectory();
    info.modTime = file.getLastWrite();
    
    file.close();
    
    xSemaphoreGive(mutex_);
    return true;
}

//==============================================================================
// Directory Operations
//==============================================================================

bool FileIO::createDir(const String& path) {
    xSemaphoreTake(mutex_, portMAX_DELAY);
    
    String fsPath;
    fs::FS* fs = getFS(path);
    resolvePath(path, fsPath);
    
    if (!fs) {
        xSemaphoreGive(mutex_);
        return false;
    }
    
    bool result = fs->mkdir(fsPath);
    
    xSemaphoreGive(mutex_);
    return result;
}

bool FileIO::deleteDir(const String& path, bool recursive) {
    xSemaphoreTake(mutex_, portMAX_DELAY);
    
    String fsPath;
    fs::FS* fs = getFS(path);
    resolvePath(path, fsPath);
    
    if (!fs) {
        xSemaphoreGive(mutex_);
        return false;
    }
    
    if (recursive) {
        // Delete contents first
        File dir = fs->open(fsPath);
        if (dir && dir.isDirectory()) {
            File file = dir.openNextFile();
            while (file) {
                String filePath = path + "/" + file.name();
                if (file.isDirectory()) {
                    deleteDir(filePath, true);
                } else {
                    fs->remove(filePath);
                }
                file = dir.openNextFile();
            }
        }
    }
    
    bool result = fs->rmdir(fsPath);
    
    xSemaphoreGive(mutex_);
    return result;
}

std::vector<FileInfo> FileIO::listDir(const String& path) {
    std::vector<FileInfo> files;
    
    xSemaphoreTake(mutex_, portMAX_DELAY);
    
    String fsPath;
    fs::FS* fs = getFS(path);
    resolvePath(path, fsPath);
    
    if (!fs) {
        xSemaphoreGive(mutex_);
        return files;
    }
    
    File dir = fs->open(fsPath);
    if (!dir || !dir.isDirectory()) {
        xSemaphoreGive(mutex_);
        return files;
    }
    
    File file = dir.openNextFile();
    while (file) {
        FileInfo info;
        info.path = path + "/" + file.name();
        info.name = file.name();
        info.size = file.size();
        info.isDirectory = file.isDirectory();
        info.modTime = file.getLastWrite();
        files.push_back(info);
        
        file = dir.openNextFile();
    }
    
    xSemaphoreGive(mutex_);
    return files;
}

//==============================================================================
// Upload Handling
//==============================================================================

std::vector<UploadedFile> FileIO::parseUpload(Request& request,
                                               const String& uploadDir,
                                               UploadProgressCallback progressCb) {
    std::vector<UploadedFile> uploadedFiles;
    
    if (!request.contentType.startsWith("multipart/form-data")) {
        LOG_WARN("FileIO", "Invalid content type for upload");
        return uploadedFiles;
    }
    
    // Get boundary
    String boundary = parseBoundary(request.contentType);
    if (boundary.isEmpty()) {
        LOG_WARN("FileIO", "No boundary found in content type");
        return uploadedFiles;
    }
    
    // Ensure upload directory exists
    createDir(uploadDir);
    
    // Parse multipart body (simplified implementation)
    String body = request.body;
    String delimiter = "--" + boundary;
    
    int pos = 0;
    while ((pos = body.indexOf(delimiter, pos)) >= 0) {
        pos += delimiter.length();
        
        if (body.substring(pos, pos + 2) == "--") {
            break; // End of multipart
        }
        
        // Skip CRLF
        if (body.charAt(pos) == '\r') pos++;
        if (body.charAt(pos) == '\n') pos++;
        
        // Read headers until empty line
        String name, filename, contentType;
        while (pos < (int)body.length()) {
            int lineEnd = body.indexOf('\n', pos);
            if (lineEnd < 0) break;
            
            String line = body.substring(pos, lineEnd);
            line.trim();
            
            if (line.isEmpty()) {
                pos = lineEnd + 1;
                break;
            }
            
            parsePartHeader(line, name, filename, contentType);
            pos = lineEnd + 1;
        }
        
        // Find end of part
        int partEnd = body.indexOf(delimiter, pos);
        if (partEnd < 0) break;
        
        // Get content (minus trailing CRLF)
        String content = body.substring(pos, partEnd - 2);
        
        if (!filename.isEmpty()) {
            // File upload
            String savePath = uploadDir + "/" + filename;
            
            if (writeFile(savePath, content)) {
                UploadedFile uploaded;
                uploaded.name = name;
                uploaded.filename = filename;
                uploaded.contentType = contentType;
                uploaded.tempPath = savePath;
                uploaded.size = content.length();
                uploadedFiles.push_back(uploaded);
                
                if (uploadCompleteCb_) {
                    uploadCompleteCb_(uploaded, true);
                }
                
                LOG_INFO("FileIO", "Uploaded: " + filename + " (" + 
                        String(content.length()) + " bytes)");
            }
        } else if (!name.isEmpty()) {
            // Form field - add to request.form
            request.form[name] = content;
        }
        
        pos = partEnd;
    }
    
    return uploadedFiles;
}

//==============================================================================
// Download Handling
//==============================================================================

Response FileIO::createDownloadResponse(const String& path, const String& filename) {
    if (!exists(path)) {
        return Response::error(HttpStatus::NOT_FOUND, "File not found");
    }
    
    return Response::file(path, filename);
}

size_t FileIO::streamFile(WiFiClient& client, const String& path,
                          size_t rangeStart, size_t rangeEnd) {
    String fsPath;
    fs::FS* fs = getFS(path);
    resolvePath(path, fsPath);
    
    if (!fs) return 0;
    
    File file = fs->open(fsPath, FILE_READ);
    if (!file) return 0;
    
    size_t fileSize = file.size();
    size_t start = rangeStart;
    size_t end = (rangeEnd > 0 && rangeEnd < fileSize) ? rangeEnd : fileSize - 1;
    size_t length = end - start + 1;
    
    file.seek(start);
    
    uint8_t buffer[512];
    size_t totalSent = 0;
    
    while (totalSent < length) {
        size_t toRead = min(sizeof(buffer), length - totalSent);
        size_t bytesRead = file.read(buffer, toRead);
        
        if (bytesRead == 0) break;
        
        size_t bytesSent = client.write(buffer, bytesRead);
        if (bytesSent == 0) break;
        
        totalSent += bytesSent;
    }
    
    file.close();
    return totalSent;
}

//==============================================================================
// Utilities
//==============================================================================

fs::FS* FileIO::getFS(const String& path) {
    if (path.startsWith("sd://") || path.startsWith("/sd/")) {
        return sdAvailable_ ? &SD : nullptr;
    } else if (path.startsWith("spiffs://") || path.startsWith("/spiffs/")) {
        return spiffsAvailable_ ? &SPIFFS : nullptr;
    }
    
    // Default: prefer SD if available
    if (sdAvailable_) return &SD;
    if (spiffsAvailable_) return &SPIFFS;
    
    return nullptr;
}

FileSystemType FileIO::resolvePath(const String& path, String& fsPath) {
    if (path.startsWith("sd://")) {
        fsPath = path.substring(5);
        return FileSystemType::SD_CARD;
    } else if (path.startsWith("/sd/")) {
        fsPath = path.substring(3);
        return FileSystemType::SD_CARD;
    } else if (path.startsWith("spiffs://")) {
        fsPath = path.substring(9);
        return FileSystemType::SPIFFS;
    } else if (path.startsWith("/spiffs/")) {
        fsPath = path.substring(7);
        return FileSystemType::SPIFFS;
    }
    
    // Use path as-is
    fsPath = path;
    return FileSystemType::AUTO;
}

size_t FileIO::getFreeSpace(FileSystemType fsType) {
    if (fsType == FileSystemType::SD_CARD && sdAvailable_) {
        return SD.totalBytes() - SD.usedBytes();
    } else if (fsType == FileSystemType::SPIFFS && spiffsAvailable_) {
        return SPIFFS.totalBytes() - SPIFFS.usedBytes();
    }
    return 0;
}

size_t FileIO::getTotalSpace(FileSystemType fsType) {
    if (fsType == FileSystemType::SD_CARD && sdAvailable_) {
        return SD.totalBytes();
    } else if (fsType == FileSystemType::SPIFFS && spiffsAvailable_) {
        return SPIFFS.totalBytes();
    }
    return 0;
}

String FileIO::parseBoundary(const String& contentType) {
    int boundaryPos = contentType.indexOf("boundary=");
    if (boundaryPos < 0) return "";
    
    String boundary = contentType.substring(boundaryPos + 9);
    boundary.trim();
    
    // Remove quotes if present
    if (boundary.startsWith("\"")) {
        boundary = boundary.substring(1);
        int endQuote = boundary.indexOf("\"");
        if (endQuote > 0) {
            boundary = boundary.substring(0, endQuote);
        }
    }
    
    return boundary;
}

void FileIO::parsePartHeader(const String& header, String& name,
                             String& filename, String& contentType) {
    if (header.startsWith("Content-Disposition:")) {
        // Parse name
        int namePos = header.indexOf("name=\"");
        if (namePos > 0) {
            namePos += 6;
            int nameEnd = header.indexOf("\"", namePos);
            if (nameEnd > 0) {
                name = header.substring(namePos, nameEnd);
            }
        }
        
        // Parse filename
        int filePos = header.indexOf("filename=\"");
        if (filePos > 0) {
            filePos += 10;
            int fileEnd = header.indexOf("\"", filePos);
            if (fileEnd > 0) {
                filename = header.substring(filePos, fileEnd);
            }
        }
    } else if (header.startsWith("Content-Type:")) {
        contentType = header.substring(13);
        contentType.trim();
    }
}

} // namespace espweb
