/**
 * @file GT911Touch.cpp
 * @brief Concrete implementation of ITouch for GT911 capacitive touch
 * 
 * @version 1.0.0 (MASTER_PLAN Phase 1)
 */

#include "GT911Touch.h"
#include <Arduino.h>

namespace HAL {

GT911Touch::GT911Touch()
    : _touch(nullptr)
    , _lastX(0)
    , _lastY(0)
    , _rawX(0)
    , _rawY(0)
    , _displayWidth(0)
    , _displayHeight(0)
    , _touched(false)
    , _initialized(false)
{
    /* Initialize calibration to defaults */
    _calibration.swapXY = false;
    _calibration.mapX1 = -1;
    _calibration.mapX2 = -1;
    _calibration.mapY1 = -1;
    _calibration.mapY2 = -1;
    _calibration.maxX = 0;
    _calibration.maxY = 0;
}

GT911Touch::~GT911Touch() {
    deinit();
}

bool GT911Touch::init(uint16_t displayWidth, uint16_t displayHeight, uint8_t rotation) {
    if (_initialized) {
        return true;
    }
    
    Serial.println("[HAL:Touch] Initializing GT911...");
    
    _displayWidth = displayWidth;
    _displayHeight = displayHeight;
    _calibration.maxX = displayWidth - 1;
    _calibration.maxY = displayHeight - 1;
    
    /* Calculate calibration based on rotation */
    calculateCalibrationForRotation(rotation);
    
    /* Hardware reset */
    pinMode(TouchConfig::PIN_RES, OUTPUT);
    digitalWrite(TouchConfig::PIN_RES, LOW);
    delay(TouchConfig::RESET_DELAY_MS);
    digitalWrite(TouchConfig::PIN_RES, HIGH);
    delay(TouchConfig::RESET_DELAY_MS);
    
    /* Create GT911 instance */
    _touch = new Touch_GT911(
        TouchConfig::PIN_SDA,
        TouchConfig::PIN_SCL,
        TouchConfig::PIN_INT,
        TouchConfig::PIN_RES,
        displayWidth,
        displayHeight
    );
    
    if (!_touch) {
        Serial.println("[HAL:Touch] Failed to create GT911 instance!");
        return false;
    }
    
    /* Initialize touch controller */
    _touch->begin(TouchConfig::I2C_ADDR_1);
    _touch->setRotation(rotation == 2 ? ROTATION_INVERTED : ROTATION_NORMAL);
    
    _initialized = true;
    Serial.printf("[HAL:Touch] Initialized for %dx%d display\n", displayWidth, displayHeight);
    
    return true;
}

void GT911Touch::deinit() {
    if (!_initialized) {
        return;
    }
    
    if (_touch) {
        delete _touch;
        _touch = nullptr;
    }
    
    _initialized = false;
    Serial.println("[HAL:Touch] Deinitialized");
}

void GT911Touch::calculateCalibrationForRotation(uint8_t rotation) {
    switch (rotation) {
        case 3:
            _calibration.swapXY = true;
            _calibration.mapX1 = _calibration.maxX;
            _calibration.mapX2 = 0;
            _calibration.mapY1 = 0;
            _calibration.mapY2 = _calibration.maxY;
            break;
        case 2:
            _calibration.swapXY = false;
            _calibration.mapX1 = _calibration.maxX;
            _calibration.mapX2 = 0;
            _calibration.mapY1 = _calibration.maxY;
            _calibration.mapY2 = 0;
            break;
        case 1:
            _calibration.swapXY = true;
            _calibration.mapX1 = 0;
            _calibration.mapX2 = _calibration.maxX;
            _calibration.mapY1 = _calibration.maxY;
            _calibration.mapY2 = 0;
            break;
        default: /* case 0 */
            _calibration.swapXY = false;
            _calibration.mapX1 = 0;
            _calibration.mapX2 = _calibration.maxX;
            _calibration.mapY1 = 0;
            _calibration.mapY2 = _calibration.maxY;
            break;
    }
}

void GT911Touch::applyCalibration() {
    if (_calibration.swapXY) {
        _lastX = map(_rawY, _calibration.mapX1, _calibration.mapX2, 0, _calibration.maxX);
        _lastY = map(_rawX, _calibration.mapY1, _calibration.mapY2, 0, _calibration.maxY);
    } else {
        _lastX = map(_rawX, _calibration.mapX1, _calibration.mapX2, 0, _calibration.maxX);
        _lastY = map(_rawY, _calibration.mapY1, _calibration.mapY2, 0, _calibration.maxY);
    }
}

bool GT911Touch::hasSignal() const {
    return _initialized;
}

bool GT911Touch::read(TouchPoint& point) {
    if (!_initialized || !_touch) {
        point.pressed = false;
        return false;
    }
    
    _touch->read();
    
    if (_touch->isTouched && _touch->touches > 0) {
        _rawX = _touch->points[0].x;
        _rawY = _touch->points[0].y;
        _lastX = _rawX;
        _lastY = _rawY;
        applyCalibration();
        
        point.x = _lastX;
        point.y = _lastY;
        point.pressed = true;
        point.id = 0;
        _touched = true;
        return true;
    }
    
    point.pressed = false;
    _touched = false;
    return false;
}

bool GT911Touch::isTouched() const {
    return _touched;
}

bool GT911Touch::isReleased() const {
    if (!_initialized || !_touch) {
        return false;
    }
    return !_touch->isTouched;
}

TouchCalibration GT911Touch::getCalibration() const {
    return _calibration;
}

void GT911Touch::setCalibration(const TouchCalibration& cal) {
    _calibration = cal;
}

bool GT911Touch::runCalibration() {
    /* TODO: Implement interactive calibration screen */
    Serial.println("[HAL:Touch] Calibration not implemented yet");
    return false;
}

void GT911Touch::lvglRead(lv_indev_drv_t* indev_drv, lv_indev_data_t* data) {
    TouchPoint point;
    
    if (hasSignal()) {
        if (read(point)) {
            data->state = LV_INDEV_STATE_PR;
            /* Invert coordinates for this display orientation */
            data->point.x = _displayWidth - _lastX;
            data->point.y = _displayHeight - _lastY;
        } else if (isReleased()) {
            data->state = LV_INDEV_STATE_REL;
        }
    } else {
        data->state = LV_INDEV_STATE_REL;
    }
}

int16_t GT911Touch::getLastX() const {
    return _lastX;
}

int16_t GT911Touch::getLastY() const {
    return _lastY;
}

bool GT911Touch::isInitialized() const {
    return _initialized;
}

} // namespace HAL
