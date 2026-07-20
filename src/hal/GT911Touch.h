/**
 * @file GT911Touch.h
 * @brief Concrete implementation of ITouch for GT911 capacitive touch
 * 
 * @version 1.0.0 (MASTER_PLAN Phase 1)
 */

#ifndef HAL_GT911_TOUCH_H
#define HAL_GT911_TOUCH_H

#include "ITouch.h"
#include "../config/defaults.h"
#include <Touch_GT911.h>

namespace HAL {

class GT911Touch : public ITouch {
public:
    GT911Touch();
    ~GT911Touch() override;
    
    /*==== ITouch Implementation ====*/
    
    bool init(uint16_t displayWidth, uint16_t displayHeight, uint8_t rotation) override;
    void deinit() override;
    
    bool hasSignal() const override;
    bool read(TouchPoint& point) override;
    bool isTouched() const override;
    bool isReleased() const override;
    
    TouchCalibration getCalibration() const override;
    void setCalibration(const TouchCalibration& cal) override;
    bool runCalibration() override;
    
    void lvglRead(lv_indev_drv_t* indev_drv, lv_indev_data_t* data) override;
    
    int16_t getLastX() const override;
    int16_t getLastY() const override;
    bool isInitialized() const override;
    
private:
    Touch_GT911* _touch;
    TouchCalibration _calibration;
    
    int16_t _lastX;
    int16_t _lastY;
    int16_t _rawX;
    int16_t _rawY;
    
    uint16_t _displayWidth;
    uint16_t _displayHeight;
    
    bool _touched;
    bool _initialized;
    
    void applyCalibration();
    void calculateCalibrationForRotation(uint8_t rotation);
};

} // namespace HAL

#endif /* HAL_GT911_TOUCH_H */
