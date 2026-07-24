/**
 * @file logger.cpp
 * @brief ESP32 Web Framework - Logger Implementation
 * 
 * @author ESP32 Web Framework
 * @version 1.0.0
 */

#include "logger.hpp"
#include "fileio.hpp"

namespace espweb {

// Singleton instance
Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

Logger::Logger() {
    mutex_ = xSemaphoreCreateMutex();
    logFilePath_ = LOG_FILE_PATH;
}

bool Logger::init() {
    LOG_INFO("Logger", "Logger initialized");
    return true;
}

void Logger::log(LogLevel level, const String& tag, const String& message,
                 const char* file, int line) {
    if (level < minLevel_) {
        return;
    }
    
    xSemaphoreTake(mutex_, portMAX_DELAY);
    
    LogEntry entry;
    entry.timestamp = millis();
    entry.level = level;
    entry.tag = tag;
    entry.message = message;
    if (file) entry.file = file;
    entry.line = line;
    
    // Write to serial
    if (serialEnabled_) {
        writeSerial(entry);
    }
    
    // Write to file
    if (fileEnabled_) {
        writeFile(entry);
    }
    
    // Call custom handlers
    for (auto& handler : handlers_) {
        handler(entry);
    }
    
    // Add to buffer
    buffer_.push_back(entry);
    if (buffer_.size() > maxBufferSize_) {
        buffer_.erase(buffer_.begin());
    }
    
    xSemaphoreGive(mutex_);
}

void Logger::trace(const String& tag, const String& message) {
    log(LogLevel::TRACE, tag, message);
}

void Logger::debug(const String& tag, const String& message) {
    log(LogLevel::DEBUG, tag, message);
}

void Logger::info(const String& tag, const String& message) {
    log(LogLevel::INFO, tag, message);
}

void Logger::warn(const String& tag, const String& message) {
    log(LogLevel::WARN, tag, message);
}

void Logger::error(const String& tag, const String& message) {
    log(LogLevel::ERROR, tag, message);
}

void Logger::fatal(const String& tag, const String& message) {
    log(LogLevel::FATAL, tag, message);
}

void Logger::addHandler(LogHandler handler) {
    xSemaphoreTake(mutex_, portMAX_DELAY);
    handlers_.push_back(handler);
    xSemaphoreGive(mutex_);
}

String Logger::format(const LogEntry& entry) {
    char timestamp[16];
    uint32_t secs = entry.timestamp / 1000;
    uint32_t mins = secs / 60;
    uint32_t hours = mins / 60;
    snprintf(timestamp, sizeof(timestamp), "%02lu:%02lu:%02lu.%03lu",
             hours % 24, mins % 60, secs % 60, entry.timestamp % 1000);
    
    String formatted = "[";
    formatted += timestamp;
    formatted += "] [";
    formatted += levelToString(entry.level);
    formatted += "] [";
    formatted += entry.tag;
    formatted += "] ";
    formatted += entry.message;
    
    if (!entry.file.isEmpty()) {
        formatted += " (";
        // Just filename, not full path
        int lastSlash = entry.file.lastIndexOf('/');
        if (lastSlash < 0) lastSlash = entry.file.lastIndexOf('\\');
        formatted += lastSlash >= 0 ? entry.file.substring(lastSlash + 1) : entry.file;
        formatted += ":";
        formatted += String(entry.line);
        formatted += ")";
    }
    
    return formatted;
}

void Logger::writeSerial(const LogEntry& entry) {
    String formatted = format(entry);
    
    if (colorsEnabled_) {
        Serial.print(levelToColor(entry.level));
        Serial.print(formatted);
        Serial.println("\033[0m");
    } else {
        Serial.println(formatted);
    }
}

void Logger::writeFile(const LogEntry& entry) {
    checkRotation();
    
    String formatted = format(entry) + "\n";
    Files().writeFile(logFilePath_, formatted, true);
}

void Logger::rotate() {
    xSemaphoreTake(mutex_, portMAX_DELAY);
    
    // Rotate log files
    for (int i = settings::LOG_MAX_FILES - 1; i > 0; i--) {
        String oldName = logFilePath_ + "." + String(i);
        String newName = logFilePath_ + "." + String(i + 1);
        Files().renameFile(oldName, newName);
    }
    
    // Rename current to .1
    Files().renameFile(logFilePath_, logFilePath_ + ".1");
    
    xSemaphoreGive(mutex_);
}

void Logger::flush() {
    // Force write buffer to file
    // In this simple implementation, logs are written immediately
}

std::vector<LogEntry> Logger::getRecent(size_t count) {
    xSemaphoreTake(mutex_, portMAX_DELAY);
    
    std::vector<LogEntry> result;
    size_t start = buffer_.size() > count ? buffer_.size() - count : 0;
    
    for (size_t i = start; i < buffer_.size(); i++) {
        result.push_back(buffer_[i]);
    }
    
    xSemaphoreGive(mutex_);
    return result;
}

void Logger::clearBuffer() {
    xSemaphoreTake(mutex_, portMAX_DELAY);
    buffer_.clear();
    xSemaphoreGive(mutex_);
}

size_t Logger::getLogFileSize() {
    return Files().getFileSize(logFilePath_);
}

void Logger::checkRotation() {
    if (getLogFileSize() > settings::LOG_MAX_FILE_SIZE) {
        rotate();
    }
}

} // namespace espweb
