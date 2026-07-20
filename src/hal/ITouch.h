/**
 * @file ITouch.h
 * @brief Touch Input Hardware Abstraction Layer Interface
 * 
 * Defines the contract for touch input implementations.
 * Supports single and multi-touch panels.
 * 
 * @version 1.0.0 (MASTER_PLAN Phase 1)
 */

#ifndef HAL_ITOUCH_H
#define HAL_ITOUCH_H

#include <cstdint>
#include <lvgl.h>

namespace HAL {

/**
 * @brief Touch point data structure
 */
struct TouchPoint {
    int16_t x;
    int16_t y;
    bool pressed;
    uint8_t id;         /* For multi-touch */
};

/**
 * @brief Touch calibration data
 */
struct TouchCalibration {
    bool swapXY;
    int16_t mapX1, mapX2;
    int16_t mapY1, mapY2;
    int16_t maxX, maxY;
};

/**
 * @brief Touch interface - abstracts hardware-specific touch operations
 */
class ITouch {
public:
    virtual ~ITouch() = default;
    
    /*==== Lifecycle ====*/
    
    /**
     * @brief Initialize touch hardware
     * @param displayWidth Display width for calibration
     * @param displayHeight Display height for calibration
     * @param rotation Display rotation for coordinate mapping
     * @return true on success
     */
    virtual bool init(uint16_t displayWidth, uint16_t displayHeight, uint8_t rotation) = 0;
    
    /**
     * @brief Deinitialize and release resources
     */
    virtual void deinit() = 0;
    
    /*==== Input Reading ====*/
    
    /**
     * @brief Check if touch signal is available
     */
    virtual bool hasSignal() const = 0;
    
    /**
     * @brief Read current touch state
     * @param point Output touch point data
     * @return true if touched
     */
    virtual bool read(TouchPoint& point) = 0;
    
    /**
     * @brief Check if currently touched
     */
    virtual bool isTouched() const = 0;
    
    /**
     * @brief Check if touch was just released
     */
    virtual bool isReleased() const = 0;
    
    /*==== Calibration ====*/
    
    /**
     * @brief Get current calibration data
     */
    virtual TouchCalibration getCalibration() const = 0;
    
    /**
     * @brief Set calibration data
     */
    virtual void setCalibration(const TouchCalibration& cal) = 0;
    
    /**
     * @brief Run interactive calibration routine
     * @return true if calibration successful
     */
    virtual bool runCalibration() = 0;
    
    /*==== LVGL Integration ====*/
    
    /**
     * @brief LVGL input device callback
     * @param indev_drv LVGL input driver
     * @param data Output touch data for LVGL
     */
    virtual void lvglRead(lv_indev_drv_t* indev_drv, lv_indev_data_t* data) = 0;
    
    /*==== Properties ====*/
    
    /**
     * @brief Get last X coordinate
     */
    virtual int16_t getLastX() const = 0;
    
    /**
     * @brief Get last Y coordinate
     */
    virtual int16_t getLastY() const = 0;
    
    /**
     * @brief Check if hardware is initialized
     */
    virtual bool isInitialized() const = 0;
};

} // namespace HAL

#endif /* HAL_ITOUCH_H */
