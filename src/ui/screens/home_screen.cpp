/**
 * @file home_screen.cpp
 * @brief Home Screen implementation
 * 
 * @version 1.0.0 (MASTER_PLAN Phase 4)
 */

#include "home_screen.h"
#include "screen_manager.h"
#include "../../config/defaults.h"
#include <Arduino.h>

namespace UI {

HomeScreen::HomeScreen()
    : ScreenBase(ScreenId::HOME)
    , _lblTitle(nullptr)
    , _lblStatus(nullptr)
    , _btnDemo(nullptr)
    , _btnSettings(nullptr)
    , _lblMemory(nullptr)
    , _lastUpdate(0)
{
}

bool HomeScreen::create() {
    if (!createScreenContainer()) {
        return false;
    }
    
    /* Title */
    _lblTitle = lv_label_create(_screen);
    lv_label_set_text(_lblTitle, SYSTEM_NAME);
    lv_obj_set_style_text_font(_lblTitle, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(_lblTitle, lv_color_hex(0xe94560), 0);
    lv_obj_align(_lblTitle, LV_ALIGN_TOP_MID, 0, 20);
    
    /* Version */
    _lblStatus = lv_label_create(_screen);
    lv_label_set_text(_lblStatus, "Version " SYSTEM_VERSION);
    lv_obj_set_style_text_color(_lblStatus, lv_color_hex(0x888888), 0);
    lv_obj_align(_lblStatus, LV_ALIGN_TOP_MID, 0, 50);
    
    /* Demo Button */
    _btnDemo = lv_btn_create(_screen);
    lv_obj_set_size(_btnDemo, 140, 50);
    lv_obj_align(_btnDemo, LV_ALIGN_CENTER, -80, 20);
    lv_obj_set_style_bg_color(_btnDemo, lv_color_hex(0x0f3460), 0);
    lv_obj_add_event_cb(_btnDemo, onDemoClick, LV_EVENT_CLICKED, this);
    
    lv_obj_t* lblDemo = lv_label_create(_btnDemo);
    lv_label_set_text(lblDemo, LV_SYMBOL_PLAY " Demo");
    lv_obj_center(lblDemo);
    
    /* Settings Button */
    _btnSettings = lv_btn_create(_screen);
    lv_obj_set_size(_btnSettings, 140, 50);
    lv_obj_align(_btnSettings, LV_ALIGN_CENTER, 80, 20);
    lv_obj_set_style_bg_color(_btnSettings, lv_color_hex(0x0f3460), 0);
    lv_obj_add_event_cb(_btnSettings, onSettingsClick, LV_EVENT_CLICKED, this);
    
    lv_obj_t* lblSettings = lv_label_create(_btnSettings);
    lv_label_set_text(lblSettings, LV_SYMBOL_SETTINGS " Settings");
    lv_obj_center(lblSettings);
    
    /* Memory label at bottom */
    _lblMemory = lv_label_create(_screen);
    lv_obj_set_style_text_color(_lblMemory, lv_color_hex(0x666666), 0);
    /* Use default font since montserrat_10 may not be enabled */
    lv_obj_align(_lblMemory, LV_ALIGN_BOTTOM_MID, 0, -10);
    updateMemoryLabel();
    
    Serial.println("[HomeScreen] Created");
    return true;
}

void HomeScreen::onEnter() {
    ScreenBase::onEnter();
    _lastUpdate = 0;  /* Force immediate update */
}

void HomeScreen::onExit() {
    ScreenBase::onExit();
}

void HomeScreen::update() {
    /* Update memory label every 2 seconds */
    uint32_t now = millis();
    if (now - _lastUpdate > 2000) {
        _lastUpdate = now;
        updateMemoryLabel();
    }
}

void HomeScreen::updateMemoryLabel() {
    if (_lblMemory) {
        uint32_t freeHeap = esp_get_free_heap_size() / 1024;
        uint32_t freePsram = esp_get_free_internal_heap_size() / 1024;
        lv_label_set_text_fmt(_lblMemory, "Heap: %luKB | Internal: %luKB",
            (unsigned long)freeHeap, (unsigned long)freePsram);
    }
}

void HomeScreen::onDemoClick(lv_event_t* e) {
    /* Navigate to demo screen */
    SCREEN_MGR.navigateTo(ScreenId::DEMO, ScreenTransition::SLIDE_LEFT);
}

void HomeScreen::onSettingsClick(lv_event_t* e) {
    /* Navigate to settings screen */
    SCREEN_MGR.navigateTo(ScreenId::SETTINGS, ScreenTransition::SLIDE_LEFT);
}

} // namespace UI
