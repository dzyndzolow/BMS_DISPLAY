/**
 * @file screen_base.cpp
 * @brief Base class implementation for UI screens
 * 
 * @version 1.0.0 (MASTER_PLAN Phase 4)
 */

#include "screen_base.h"
#include <Arduino.h>

namespace UI {

ScreenBase::ScreenBase(ScreenId id)
    : _id(id)
    , _screen(nullptr)
    , _active(false)
{
}

ScreenBase::~ScreenBase() {
    destroy();
}

void ScreenBase::onEnter() {
    _active = true;
    Serial.printf("[UI:%s] onEnter\n", getName());
}

void ScreenBase::onExit() {
    _active = false;
    Serial.printf("[UI:%s] onExit\n", getName());
}

void ScreenBase::update() {
    /* Default: do nothing. Override in derived classes for animations, etc. */
}

void ScreenBase::destroy() {
    if (_screen) {
        lv_obj_del(_screen);
        _screen = nullptr;
        Serial.printf("[UI:%s] Destroyed\n", getName());
    }
}

lv_obj_t* ScreenBase::createScreenContainer() {
    _screen = lv_obj_create(nullptr);
    if (_screen) {
        lv_obj_set_size(_screen, LV_HOR_RES, LV_VER_RES);
        lv_obj_set_style_bg_color(_screen, lv_color_hex(0x1a1a2e), 0);
        lv_obj_clear_flag(_screen, LV_OBJ_FLAG_SCROLLABLE);
    }
    return _screen;
}

lv_obj_t* ScreenBase::addTitleBar(const char* title) {
    if (!_screen) return nullptr;
    
    /* Create title bar container */
    lv_obj_t* titleBar = lv_obj_create(_screen);
    lv_obj_set_size(titleBar, LV_HOR_RES, 40);
    lv_obj_align(titleBar, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(titleBar, lv_color_hex(0x16213e), 0);
    lv_obj_set_style_border_width(titleBar, 0, 0);
    lv_obj_set_style_radius(titleBar, 0, 0);
    lv_obj_clear_flag(titleBar, LV_OBJ_FLAG_SCROLLABLE);
    
    /* Create title label */
    lv_obj_t* label = lv_label_create(titleBar);
    lv_label_set_text(label, title);
    lv_obj_set_style_text_color(label, lv_color_hex(0xffffff), 0);
    lv_obj_center(label);
    
    return titleBar;
}

lv_obj_t* ScreenBase::addBackButton() {
    if (!_screen) return nullptr;
    
    lv_obj_t* btn = lv_btn_create(_screen);
    lv_obj_set_size(btn, 60, 30);
    lv_obj_align(btn, LV_ALIGN_TOP_LEFT, 5, 5);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x0f3460), 0);
    
    lv_obj_t* label = lv_label_create(btn);
    lv_label_set_text(label, LV_SYMBOL_LEFT " Back");
    lv_obj_center(label);
    
    return btn;
}

} // namespace UI
