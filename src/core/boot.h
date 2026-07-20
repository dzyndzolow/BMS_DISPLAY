/**
 * @file boot.h
 * @brief Unified Boot Sequence - Single initialization point
 * 
 * Eliminates duplicate init code. All hardware and LVGL initialization
 * happens here in a controlled, deterministic order.
 * 
 * @version 1.0.0 (MASTER_PLAN Phase 2)
 */

#ifndef CORE_BOOT_H
#define CORE_BOOT_H

#include <cstdint>

namespace Core {

/**
 * @brief Boot stages for progress tracking
 */
enum class BootStage : uint8_t {
    START           = 0,
    SERIAL_INIT     = 1,
    STORAGE_INIT    = 2,
    LOGGER_INIT     = 3,
    DISPLAY_INIT    = 4,
    TOUCH_INIT      = 5,
    LVGL_INIT       = 6,
    UI_INIT         = 7,
    TASKS_INIT      = 8,
    COMPLETE        = 9,
    FAILED          = 255
};

/**
 * @brief Boot result structure
 */
struct BootResult {
    bool success;
    BootStage failedStage;
    const char* errorMessage;
};

/**
 * @brief Boot configuration options
 */
struct BootConfig {
    bool enableSD;
    bool enableLogging;
    bool enableTouch;
    bool startDemo;
    uint8_t displayBrightness;
};

/**
 * @brief Get default boot configuration
 */
BootConfig getDefaultBootConfig();

/**
 * @brief Run the complete boot sequence
 * @param config Boot configuration
 * @return Boot result with success/failure info
 */
BootResult boot(const BootConfig& config);

/**
 * @brief Run boot with default configuration
 */
BootResult boot();

/**
 * @brief Get current boot stage (for splash screen progress)
 */
BootStage getCurrentBootStage();

/**
 * @brief Get human-readable name for boot stage
 */
const char* getBootStageName(BootStage stage);

/**
 * @brief Emergency shutdown (call on critical failure)
 */
void emergencyShutdown(const char* reason);

} // namespace Core

#endif /* CORE_BOOT_H */
