/**
 * @file IDisplay.h
 * @brief Display Hardware Abstraction Layer Interface
 * 
 * Defines the contract for display implementations.
 * Enables testing, mocking, and swapping display drivers.
 * 
 * @version 1.0.0 (MASTER_PLAN Phase 1)
 */

#ifndef HAL_IDISPLAY_H
#define HAL_IDISPLAY_H

#include <cstdint>
#include <lvgl.h>

namespace HAL {

/**
 * @brief Display interface - abstracts hardware-specific display operations
 */
class IDisplay {
public:
    virtual ~IDisplay() = default;
    
    /*==== Lifecycle ====*/
    
    /**
     * @brief Initialize display hardware
     * @return true on success, false on failure
     */
    virtual bool init() = 0;
    
    /**
     * @brief Deinitialize and release resources
     */
    virtual void deinit() = 0;
    
    /*==== Properties ====*/
    
    /**
     * @brief Get display width in pixels
     */
    virtual uint16_t getWidth() const = 0;
    
    /**
     * @brief Get display height in pixels
     */
    virtual uint16_t getHeight() const = 0;
    
    /**
     * @brief Get current rotation (0-3)
     */
    virtual uint8_t getRotation() const = 0;
    
    /**
     * @brief Set display rotation
     * @param rotation 0=0°, 1=90°, 2=180°, 3=270°
     */
    virtual void setRotation(uint8_t rotation) = 0;
    
    /*==== Backlight ====*/
    
    /**
     * @brief Set backlight brightness
     * @param percent 0-100 brightness percentage
     */
    virtual void setBrightness(uint8_t percent) = 0;
    
    /**
     * @brief Get current brightness
     * @return 0-100 percentage
     */
    virtual uint8_t getBrightness() const = 0;
    
    /*==== LVGL Integration ====*/
    
    /**
     * @brief LVGL flush callback - sends pixels to display
     * @param disp LVGL display driver
     * @param area Area to flush
     * @param color_p Pixel data
     */
    virtual void flush(lv_disp_drv_t* disp, const lv_area_t* area, lv_color_t* color_p) = 0;
    
    /**
     * @brief Flush entire frame (for Canvas/buffered modes)
     */
    virtual void flushFrame() = 0;
    
    /*==== Low-level ====*/
    
    /**
     * @brief Fill entire screen with color
     * @param color RGB565 color value
     */
    virtual void fillScreen(uint16_t color) = 0;
    
    /**
     * @brief Check if display is initialized
     */
    virtual bool isInitialized() const = 0;
};

} // namespace HAL

#endif /* HAL_IDISPLAY_H */
