/*
 * Professional Logging System for ESP32
 * Features: Log levels, timestamps, file rotation, SD card logging
 */
#ifndef LOGGER_H
#define LOGGER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>

/*Log levels*/
typedef enum {
    LOG_LEVEL_DEBUG = 0,
    LOG_LEVEL_INFO  = 1,
    LOG_LEVEL_WARN  = 2,
    LOG_LEVEL_ERROR = 3,
    LOG_LEVEL_FATAL = 4,
    LOG_LEVEL_NONE  = 5
} log_level_t;

/*Initialize logger - call after SD card is mounted*/
bool logger_init(void);

/*Deinitialize logger - flush and close files*/
void logger_deinit(void);

/*Set minimum log level (messages below this level are ignored)*/
void logger_set_level(log_level_t level);

/*Get current log level*/
log_level_t logger_get_level(void);

/*Enable/disable Serial output*/
void logger_set_serial_output(bool enable);

/*Enable/disable SD card file output*/
void logger_set_file_output(bool enable);

/*Set maximum log file size in bytes (triggers rotation)*/
void logger_set_max_file_size(uint32_t size);

/*Set maximum number of log files to keep*/
void logger_set_max_files(uint8_t count);

/*Flush pending logs to file*/
void logger_flush(void);

/*Log functions with different levels*/
void log_debug(const char * tag, const char * format, ...);
void log_info(const char * tag, const char * format, ...);
void log_warn(const char * tag, const char * format, ...);
void log_error(const char * tag, const char * format, ...);
void log_fatal(const char * tag, const char * format, ...);

/*Generic log function*/
void log_write(log_level_t level, const char * tag, const char * format, ...);
void log_write_v(log_level_t level, const char * tag, const char * format, va_list args);

/*Get log level name string*/
const char * log_level_name(log_level_t level);

#ifdef __cplusplus
}
#endif

#endif /*LOGGER_H*/
