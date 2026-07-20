/**
 * @file NV3041ADisplay.cpp
 * @brief Concrete implementation of IDisplay for NV3041A via QSPI
 * 
 * @version 1.0.0 (MASTER_PLAN Phase 1)
 */

#include "NV3041ADisplay.h"
#include <Arduino.h>

namespace HAL {

NV3041ADisplay::NV3041ADisplay()
    : _bus(nullptr)
    , _driver(nullptr)
    , _gfx(nullptr)
    , _rotation(DisplayConfig::ROTATION)
    , _brightness(100)
    , _initialized(false)
{
}

NV3041ADisplay::~NV3041ADisplay() {
    deinit();
}

bool NV3041ADisplay::init() {
    if (_initialized) {
        return true;
    }
    
    Serial.println("[HAL:Display] Initializing NV3041A...");
    
    /* Create QSPI bus */
    _bus = new Arduino_ESP32QSPI(
        DisplayConfig::PIN_CS,
        DisplayConfig::PIN_SCK,
        DisplayConfig::PIN_D0,
        DisplayConfig::PIN_D1,
        DisplayConfig::PIN_D2,
        DisplayConfig::PIN_D3
    );
    
    if (!_bus) {
        Serial.println("[HAL:Display] Failed to create QSPI bus!");
        return false;
    }
    
    /* Create NV3041A driver */
    _driver = new Arduino_NV3041A(
        _bus,
        GFX_NOT_DEFINED,            /* RST pin - not used */
        _rotation,
        DisplayConfig::IPS_PANEL
    );
    
    if (!_driver) {
        Serial.println("[HAL:Display] Failed to create NV3041A driver!");
        delete _bus;
        _bus = nullptr;
        return false;
    }
    
    /* Create Canvas wrapper for buffered drawing */
    _gfx = new Arduino_Canvas(
        DisplayConfig::WIDTH,
        DisplayConfig::HEIGHT,
        _driver
    );
    
    if (!_gfx) {
        Serial.println("[HAL:Display] Failed to create Canvas!");
        delete _driver;
        delete _bus;
        _driver = nullptr;
        _bus = nullptr;
        return false;
    }
    
    /* Initialize GFX */
    if (!_gfx->begin()) {
        Serial.println("[HAL:Display] GFX begin() failed!");
        delete _gfx;
        delete _driver;
        delete _bus;
        _gfx = nullptr;
        _driver = nullptr;
        _bus = nullptr;
        return false;
    }
    
    /* Fill with black initially */
    _gfx->fillScreen(DisplayConfig::COLOR_BLACK);
    
    /* Initialize backlight */
    initBacklight();
    setBrightness(100);
    
    _initialized = true;
    Serial.printf("[HAL:Display] Initialized %dx%d\n", getWidth(), getHeight());
    
    return true;
}

void NV3041ADisplay::deinit() {
    if (!_initialized) {
        return;
    }
    
    setBrightness(0);
    
    if (_gfx) {
        delete _gfx;
        _gfx = nullptr;
    }
    if (_driver) {
        delete _driver;
        _driver = nullptr;
    }
    if (_bus) {
        delete _bus;
        _bus = nullptr;
    }
    
    _initialized = false;
    Serial.println("[HAL:Display] Deinitialized");
}

uint16_t NV3041ADisplay::getWidth() const {
    return _gfx ? _gfx->width() : DisplayConfig::WIDTH;
}

uint16_t NV3041ADisplay::getHeight() const {
    return _gfx ? _gfx->height() : DisplayConfig::HEIGHT;
}

uint8_t NV3041ADisplay::getRotation() const {
    return _rotation;
}

void NV3041ADisplay::setRotation(uint8_t rotation) {
    if (_gfx && rotation <= 3) {
        _rotation = rotation;
        _gfx->setRotation(rotation);
    }
}

void NV3041ADisplay::initBacklight() {
    ledcAttach(DisplayConfig::PIN_BL, DisplayConfig::BL_PWM_FREQ, DisplayConfig::BL_PWM_RES);
}

void NV3041ADisplay::setBrightness(uint8_t percent) {
    if (percent > 100) percent = 100;
    _brightness = percent;
    uint8_t duty = (uint8_t)((uint16_t)percent * 255 / 100);
    ledcWrite(DisplayConfig::PIN_BL, duty);
}

uint8_t NV3041ADisplay::getBrightness() const {
    return _brightness;
}

void NV3041ADisplay::flush(lv_disp_drv_t* disp, const lv_area_t* area, lv_color_t* color_p) {
    if (!_gfx) {
        lv_disp_flush_ready(disp);
        return;
    }
    
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);
    
#if (LV_COLOR_16_SWAP != 0)
    _gfx->draw16bitBeRGBBitmap(area->x1, area->y1, (uint16_t*)&color_p->full, w, h);
#else
    _gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t*)&color_p->full, w, h);
#endif
    
    lv_disp_flush_ready(disp);
}

void NV3041ADisplay::flushFrame() {
    if (_gfx) {
        _gfx->flush();
    }
}

void NV3041ADisplay::fillScreen(uint16_t color) {
    if (_gfx) {
        _gfx->fillScreen(color);
    }
}

bool NV3041ADisplay::isInitialized() const {
    return _initialized;
}

} // namespace HAL
