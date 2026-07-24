/**
 * @file logger.hpp
 * @brief ESP32 Web Framework - Logging System
 * 
 * Logging system with:
 * - Multiple log levels
 * - Serial and file output
 * - Log rotation
 * - Formatted messages
 * 
 * @author ESP32 Web Framework
 * @version 1.0.0
 */

#ifndef ESP_WEB_FRAMEWORK_LOGGER_HPP
#define ESP_WEB_FRAMEWORK_LOGGER_HPP

#include <Arduino.h>
#include <vector>
#include <functional>

#include "settings.h"

namespace espweb {

/**
 * @brief Log levels
 */
enum class LogLevel {
    TRACE = 0,
    DEBUG = 1,
    INFO = 2,
    WARN = 3,
    ERROR = 4,
    FATAL = 5,
    NONE = 6
};

/**
 * @brief Log entry structure
 */
struct LogEntry {
    uint32_t timestamp;
    LogLevel level;
    String tag;
    String message;
    String file;
    int line;
};

/**
 * @brief Log output handler
 */
using LogHandler = std::function<void(const LogEntry&)>;

/**
 * @brief Convert log level to string
 */
inline const char* levelToString(LogLevel level) {
    switch (level) {
        case LogLevel::TRACE: return "TRACE";
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO:  return "INFO";
        case LogLevel::WARN:  return "WARN";
        case LogLevel::ERROR: return "ERROR";
        case LogLevel::FATAL: return "FATAL";
        default: return "UNKNOWN";
    }
}

/**
 * @brief Convert log level to color code
 */
inline const char* levelToColor(LogLevel level) {
    switch (level) {
        case LogLevel::TRACE: return "\033[37m";    // White
        case LogLevel::DEBUG: return "\033[36m";    // Cyan
        case LogLevel::INFO:  return "\033[32m";    // Green
        case LogLevel::WARN:  return "\033[33m";    // Yellow
        case LogLevel::ERROR: return "\033[31m";    // Red
        case LogLevel::FATAL: return "\033[35m";    // Magenta
        default: return "\033[0m";
    }
}

/**
 * @brief Logger class - Singleton
 * 
 * Central logging facility for the framework.
 */
class Logger {
public:
    /**
     * @brief Get singleton instance
     */
    static Logger& getInstance();
    
    // Delete copy constructor and assignment
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    
    /**
     * @brief Initialize logger
     * @return true if successful
     */
    bool init();
    
    //==========================================================================
    // Configuration
    //==========================================================================
    
    /**
     * @brief Set minimum log level
     * @param level Minimum level to log
     */
    void setLevel(LogLevel level) { minLevel_ = level; }
    
    /**
     * @brief Get minimum log level
     */
    LogLevel getLevel() const { return minLevel_; }
    
    /**
     * @brief Enable/disable serial output
     */
    void setSerialEnabled(bool enabled) { serialEnabled_ = enabled; }
    
    /**
     * @brief Enable/disable file output
     */
    void setFileEnabled(bool enabled) { fileEnabled_ = enabled; }
    
    /**
     * @brief Enable/disable colors in serial output
     */
    void setColorsEnabled(bool enabled) { colorsEnabled_ = enabled; }
    
    /**
     * @brief Set log file path
     */
    void setLogFile(const String& path) { logFilePath_ = path; }
    
    /**
     * @brief Add custom log handler
     * @param handler Handler function
     */
    void addHandler(LogHandler handler);
    
    //==========================================================================
    // Logging Methods
    //==========================================================================
    
    /**
     * @brief Log message
     * @param level Log level
     * @param tag Log tag/category
     * @param message Log message
     * @param file Source file (optional)
     * @param line Source line (optional)
     */
    void log(LogLevel level, const String& tag, const String& message,
             const char* file = nullptr, int line = 0);
    
    /**
     * @brief Log trace message
     */
    void trace(const String& tag, const String& message);
    
    /**
     * @brief Log debug message
     */
    void debug(const String& tag, const String& message);
    
    /**
     * @brief Log info message
     */
    void info(const String& tag, const String& message);
    
    /**
     * @brief Log warning message
     */
    void warn(const String& tag, const String& message);
    
    /**
     * @brief Log error message
     */
    void error(const String& tag, const String& message);
    
    /**
     * @brief Log fatal message
     */
    void fatal(const String& tag, const String& message);
    
    //==========================================================================
    // Log Management
    //==========================================================================
    
    /**
     * @brief Rotate log file
     */
    void rotate();
    
    /**
     * @brief Flush log to file
     */
    void flush();
    
    /**
     * @brief Get recent log entries
     * @param count Number of entries
     * @return Vector of log entries
     */
    std::vector<LogEntry> getRecent(size_t count = 100);
    
    /**
     * @brief Clear log buffer
     */
    void clearBuffer();
    
    /**
     * @brief Get log file size
     */
    size_t getLogFileSize();
    
private:
    Logger();
    ~Logger() = default;
    
    /**
     * @brief Format log entry
     */
    String format(const LogEntry& entry);
    
    /**
     * @brief Write to serial
     */
    void writeSerial(const LogEntry& entry);
    
    /**
     * @brief Write to file
     */
    void writeFile(const LogEntry& entry);
    
    /**
     * @brief Check if rotation needed
     */
    void checkRotation();
    
    LogLevel minLevel_ = LogLevel::DEBUG;
    bool serialEnabled_ = true;
    bool fileEnabled_ = false;
    bool colorsEnabled_ = true;
    String logFilePath_;
    
    std::vector<LogHandler> handlers_;
    std::vector<LogEntry> buffer_;
    size_t maxBufferSize_ = 1000;
    
    SemaphoreHandle_t mutex_;
};

/**
 * @brief Convenience function to get logger
 */
inline Logger& Log() {
    return Logger::getInstance();
}

//==============================================================================
// Logging Macros
//==============================================================================

#define LOG_TRACE(tag, msg) espweb::Log().log(espweb::LogLevel::TRACE, tag, msg, __FILE__, __LINE__)
#define LOG_DEBUG(tag, msg) espweb::Log().log(espweb::LogLevel::DEBUG, tag, msg, __FILE__, __LINE__)
#define LOG_INFO(tag, msg)  espweb::Log().log(espweb::LogLevel::INFO, tag, msg, __FILE__, __LINE__)
#define LOG_WARN(tag, msg)  espweb::Log().log(espweb::LogLevel::WARN, tag, msg, __FILE__, __LINE__)
#define LOG_ERROR(tag, msg) espweb::Log().log(espweb::LogLevel::ERROR, tag, msg, __FILE__, __LINE__)
#define LOG_FATAL(tag, msg) espweb::Log().log(espweb::LogLevel::FATAL, tag, msg, __FILE__, __LINE__)

} // namespace espweb

#endif // ESP_WEB_FRAMEWORK_LOGGER_HPP
