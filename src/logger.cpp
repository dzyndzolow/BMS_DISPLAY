/*
 * Professional Logging System for ESP32
 * Implementation with file rotation, timestamps, multi-output
 */
#include "logger.h"
#include "sd_card.h"
#include <Arduino.h>
#include <SD.h>
#include <time.h>
#include <sys/time.h>

/*Configuration defaults*/
#define LOG_DIR             "/logs"
#define LOG_FILE_PREFIX     "log_"
#define LOG_FILE_EXT        ".txt"
#define LOG_MAX_FILE_SIZE   (512 * 1024)  /*512KB per file*/
#define LOG_MAX_FILES       5             /*Keep last 5 files*/
#define LOG_BUFFER_SIZE     256           /*Max message length*/
#define LOG_FLUSH_INTERVAL  10000         /*Flush every 10s*/

/*Logger state*/
static struct {
    bool initialized;
    log_level_t min_level;
    bool serial_output;
    bool file_output;
    uint32_t max_file_size;
    uint8_t max_files;
    File log_file;
    char current_filename[64];
    uint32_t current_file_size;
    uint32_t last_flush_time;
    SemaphoreHandle_t mutex;
} logger = {
    .initialized = false,
    .min_level = LOG_LEVEL_DEBUG,
    .serial_output = true,
    .file_output = true,
    .max_file_size = LOG_MAX_FILE_SIZE,
    .max_files = LOG_MAX_FILES,
    .current_file_size = 0,
    .last_flush_time = 0,
    .mutex = NULL
};

/*Level names*/
static const char * level_names[] = {
    "DEBUG", "INFO", "WARN", "ERROR", "FATAL", "NONE"
};

/*Level colors for Serial (ANSI)*/
static const char * level_colors[] = {
    "\033[36m",  /*DEBUG - cyan*/
    "\033[32m",  /*INFO - green*/
    "\033[33m",  /*WARN - yellow*/
    "\033[31m",  /*ERROR - red*/
    "\033[35m",  /*FATAL - magenta*/
    "\033[0m"    /*NONE - reset*/
};
#define COLOR_RESET "\033[0m"

/*Forward declarations*/
static void rotate_logs(void);
static void create_new_log_file(void);
static void get_timestamp(char * buffer, size_t size);
static void ensure_log_dir(void);

const char * log_level_name(log_level_t level) {
    if(level > LOG_LEVEL_NONE) level = LOG_LEVEL_NONE;
    return level_names[level];
}

bool logger_init(void) {
    if(logger.initialized) return true;
    
    /*Create mutex for thread safety*/
    logger.mutex = xSemaphoreCreateMutex();
    if(!logger.mutex) {
        Serial.println("[LOGGER] Failed to create mutex");
        return false;
    }
    
    /*Check if SD is available for file logging*/
    bool sd_available = sd_card_is_mounted();
    Serial.printf("[LOGGER] SD card mounted: %d\n", sd_available);
    
    if(sd_available) {
        ensure_log_dir();
        create_new_log_file();
        
        if(logger.log_file) {
            Serial.println("[LOGGER] File logging enabled");
        } else {
            Serial.println("[LOGGER] File logging failed - using Serial only");
            logger.file_output = false;
        }
    } else {
        logger.file_output = false;
        Serial.println("[LOGGER] SD not mounted, file logging disabled");
    }
    
    logger.initialized = true;
    log_info("LOGGER", "Logger initialized (level=%s, serial=%d, file=%d)",
             log_level_name(logger.min_level), 
             logger.serial_output, 
             logger.file_output);
    
    return true;
}

void logger_deinit(void) {
    if(!logger.initialized) return;
    
    logger_flush();
    
    if(logger.log_file) {
        logger.log_file.close();
    }
    
    if(logger.mutex) {
        vSemaphoreDelete(logger.mutex);
        logger.mutex = NULL;
    }
    
    logger.initialized = false;
}

void logger_set_level(log_level_t level) {
    logger.min_level = level;
}

log_level_t logger_get_level(void) {
    return logger.min_level;
}

void logger_set_serial_output(bool enable) {
    logger.serial_output = enable;
}

void logger_set_file_output(bool enable) {
    if(enable && !sd_card_is_mounted()) {
        Serial.println("[LOGGER] Cannot enable file output - SD not mounted");
        return;
    }
    logger.file_output = enable;
}

void logger_set_max_file_size(uint32_t size) {
    logger.max_file_size = size;
}

void logger_set_max_files(uint8_t count) {
    logger.max_files = count;
}

void logger_flush(void) {
    if(!logger.initialized || !logger.file_output) return;
    
    if(xSemaphoreTake(logger.mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        if(logger.log_file) {
            logger.log_file.flush();
        }
        logger.last_flush_time = millis();
        xSemaphoreGive(logger.mutex);
    }
}

static void ensure_log_dir(void) {
    if(!SD.exists(LOG_DIR)) {
        if(SD.mkdir(LOG_DIR)) {
            Serial.printf("[LOGGER] Created log directory: %s\n", LOG_DIR);
        } else {
            Serial.printf("[LOGGER] Failed to create log directory: %s\n", LOG_DIR);
        }
    } else {
        Serial.printf("[LOGGER] Log directory exists: %s\n", LOG_DIR);
    }
}

static void create_new_log_file(void) {
    /*Generate filename with timestamp*/
    struct tm timeinfo;
    time_t now;
    time(&now);
    localtime_r(&now, &timeinfo);
    
    /*Format: /logs/log_YYYYMMDD_HHMMSS.txt*/
    snprintf(logger.current_filename, sizeof(logger.current_filename),
             "%s/%s%04d%02d%02d_%02d%02d%02d%s",
             LOG_DIR, LOG_FILE_PREFIX,
             timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
             timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec,
             LOG_FILE_EXT);
    
    /*If time not set, use boot counter*/
    if(timeinfo.tm_year < 100) {
        snprintf(logger.current_filename, sizeof(logger.current_filename),
                 "%s/%s%lu%s", LOG_DIR, LOG_FILE_PREFIX, millis(), LOG_FILE_EXT);
    }
    
    logger.log_file = SD.open(logger.current_filename, FILE_WRITE);
    if(!logger.log_file) {
        Serial.printf("[LOGGER] Failed to create log file: %s\n", logger.current_filename);
        logger.file_output = false;
        return;
    }
    
    logger.current_file_size = 0;
    Serial.printf("[LOGGER] Created log file: %s\n", logger.current_filename);
    
    /*Write header*/
    logger.log_file.println("=== ESP32 Log File ===");
    logger.log_file.printf("Created: %s\n", logger.current_filename);
    logger.log_file.println("Format: [TIMESTAMP] [LEVEL] [TAG] Message");
    logger.log_file.println("==========================================");
    logger.log_file.flush();
}

static void rotate_logs(void) {
    /*Close current file*/
    if(logger.log_file) {
        logger.log_file.close();
    }
    
    /*Delete oldest files if we have too many*/
    File dir = SD.open(LOG_DIR);
    if(!dir) return;
    
    /*Count log files and find oldest*/
    int count = 0;
    char oldest_file[64] = {0};
    uint32_t oldest_time = UINT32_MAX;
    
    File file = dir.openNextFile();
    while(file) {
        const char * name = file.name();
        if(strstr(name, LOG_FILE_PREFIX) != NULL) {
            count++;
            /*Use file creation order based on name*/
            if(oldest_file[0] == 0 || strcmp(name, oldest_file) < 0) {
                strncpy(oldest_file, name, sizeof(oldest_file) - 1);
            }
        }
        file = dir.openNextFile();
    }
    dir.close();
    
    /*Delete oldest if over limit*/
    while(count >= logger.max_files && oldest_file[0] != 0) {
        char full_path[80];
        snprintf(full_path, sizeof(full_path), "%s/%s", LOG_DIR, oldest_file);
        SD.remove(full_path);
        Serial.printf("[LOGGER] Deleted old log: %s\n", full_path);
        count--;
        oldest_file[0] = 0;  /*Reset for next iteration if needed*/
    }
    
    /*Create new log file*/
    create_new_log_file();
}

static void get_timestamp(char * buffer, size_t size) {
    struct tm timeinfo;
    time_t now;
    time(&now);
    localtime_r(&now, &timeinfo);
    
    if(timeinfo.tm_year < 100) {
        /*Time not set - use uptime*/
        uint32_t uptime = millis() / 1000;
        snprintf(buffer, size, "%02lu:%02lu:%02lu", 
                 uptime / 3600, (uptime % 3600) / 60, uptime % 60);
    } else {
        snprintf(buffer, size, "%04d-%02d-%02d %02d:%02d:%02d",
                 timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
                 timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    }
}

void log_write_v(log_level_t level, const char * tag, const char * format, va_list args) {
    if(!logger.initialized) {
        /*Fallback to Serial only*/
        if(level >= LOG_LEVEL_WARN) {
            Serial.printf("[%s] [%s] ", log_level_name(level), tag);
            char buf[LOG_BUFFER_SIZE];
            vsnprintf(buf, sizeof(buf), format, args);
            Serial.println(buf);
        }
        return;
    }
    
    if(level < logger.min_level) return;
    
    char timestamp[32];
    get_timestamp(timestamp, sizeof(timestamp));
    
    char message[LOG_BUFFER_SIZE];
    vsnprintf(message, sizeof(message), format, args);
    
    /*Thread-safe logging*/
    if(xSemaphoreTake(logger.mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        /*Serial output with colors*/
        if(logger.serial_output) {
            Serial.printf("%s[%s] [%-5s] [%s] %s%s\n",
                         level_colors[level], timestamp, 
                         log_level_name(level), tag, message, COLOR_RESET);
        }
        
        /*File output*/
        if(logger.file_output && logger.log_file) {
            int written = logger.log_file.printf("[%s] [%-5s] [%s] %s\n",
                                                  timestamp, log_level_name(level), 
                                                  tag, message);
            logger.current_file_size += written;
            Serial.printf("[LOGGER_WRITE] Wrote %d bytes, tag=%s, level=%s\n", written, tag, log_level_name(level));
            
            /*Check for rotation*/
            if(logger.current_file_size >= logger.max_file_size) {
                rotate_logs();
            }
            
            /*Periodic flush*/
            if(millis() - logger.last_flush_time > LOG_FLUSH_INTERVAL) {
                logger.log_file.flush();
                logger.last_flush_time = millis();
            }
        }
        
        xSemaphoreGive(logger.mutex);
    }
}

void log_write(log_level_t level, const char * tag, const char * format, ...) {
    va_list args;
    va_start(args, format);
    log_write_v(level, tag, format, args);
    va_end(args);
}

void log_debug(const char * tag, const char * format, ...) {
    va_list args;
    va_start(args, format);
    log_write_v(LOG_LEVEL_DEBUG, tag, format, args);
    va_end(args);
}

void log_info(const char * tag, const char * format, ...) {
    va_list args;
    va_start(args, format);
    log_write_v(LOG_LEVEL_INFO, tag, format, args);
    va_end(args);
}

void log_warn(const char * tag, const char * format, ...) {
    va_list args;
    va_start(args, format);
    log_write_v(LOG_LEVEL_WARN, tag, format, args);
    va_end(args);
}

void log_error(const char * tag, const char * format, ...) {
    va_list args;
    va_start(args, format);
    log_write_v(LOG_LEVEL_ERROR, tag, format, args);
    va_end(args);
}

void log_fatal(const char * tag, const char * format, ...) {
    va_list args;
    va_start(args, format);
    log_write_v(LOG_LEVEL_FATAL, tag, format, args);
    va_end(args);
    
    /*Force flush on fatal*/
    logger_flush();
}
