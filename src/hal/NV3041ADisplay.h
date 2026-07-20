/**
 * @file NV3041ADisplay.h
 * @brief Concrete implementation of IDisplay for NV3041A via QSPI
 * 
 * @version 1.0.0 (MASTER_PLAN Phase 1)
 */

#ifndef HAL_NV3041A_DISPLAY_H
#define HAL_NV3041A_DISPLAY_H

#include "IDisplay.h"
#include "../config/defaults.h"
#include <Arduino_GFX_Library.h>

namespace HAL {

class NV3041ADisplay : public IDisplay {
public:
    NV3041ADisplay();
    ~NV3041ADisplay() override;
    
    /*==== IDisplay Implementation ====*/
    
    bool init() override;
    void deinit() override;
    
    uint16_t getWidth() const override;
    uint16_t getHeight() const override;
    uint8_t getRotation() const override;
    void setRotation(uint8_t rotation) override;
    
    void setBrightness(uint8_t percent) override;
    uint8_t getBrightness() const override;
    
    void flush(lv_disp_drv_t* disp, const lv_area_t* area, lv_color_t* color_p) override;
    void flushFrame() override;
    
    void fillScreen(uint16_t color) override;
    bool isInitialized() const override;
    
    /*==== NV3041A Specific ====*/
    
    /**
     * @brief Get underlying GFX object (for advanced operations)
     */
    Arduino_GFX* getGfx() { return _gfx; }
    
private:
    Arduino_DataBus* _bus;
    Arduino_GFX* _driver;
    Arduino_GFX* _gfx;      /* Canvas wrapper */
    
    uint8_t _rotation;
    uint8_t _brightness;
    bool _initialized;
    
    void initBacklight();
};

} // namespace HAL

#endif /* HAL_NV3041A_DISPLAY_H */
