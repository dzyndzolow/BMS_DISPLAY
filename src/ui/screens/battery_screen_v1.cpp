/**
 * @file battery_screen_v1.cpp
 * @brief Battery Monitor Screen V1 implementation
 * 
 * Large vertical battery icon with voltage, current, power and status.
 * Black background, colored accents inspired by JK BMS V19 display.
 * 
 * @version 1.0.0
 */

#include "battery_screen_v1.h"
#include "screen_manager.h"
#include <Arduino.h>

namespace UI {

/*===========================================================================*/
/*                          COLOR PALETTE                                     */
/*===========================================================================*/

/* Background colors */
static const lv_color_t COL_BG         = lv_color_hex(0x000000);  /* Pure black */
static const lv_color_t COL_PANEL      = lv_color_hex(0x1A1A1A);  /* Dark grey panels */
static const lv_color_t COL_PANEL_BRD  = lv_color_hex(0x2A2A2A);  /* Panel border */

/* Accent colors */
static const lv_color_t COL_CYAN       = lv_color_hex(0x00E5FF);  /* Voltage */
static const lv_color_t COL_GREEN      = lv_color_hex(0x00E676);  /* Charging / Good */
static const lv_color_t COL_RED        = lv_color_hex(0xFF5252);  /* Discharging / Low */
static const lv_color_t COL_YELLOW     = lv_color_hex(0xFFD740);  /* Power / Warning */
static const lv_color_t COL_ORANGE     = lv_color_hex(0xFF9100);  /* Medium SOC */
static const lv_color_t COL_WHITE      = lv_color_hex(0xFFFFFF);  /* Text */
static const lv_color_t COL_GREY       = lv_color_hex(0x888888);  /* Dim text */
static const lv_color_t COL_DARK_GREY  = lv_color_hex(0x333333);  /* Battery empty area */

/* Battery icon dimensions */
static const int16_t BAT_BODY_W     = 70;
static const int16_t BAT_BODY_H     = 160;
static const int16_t BAT_CAP_W      = 30;
static const int16_t BAT_CAP_H      = 10;
static const int16_t BAT_FILL_PAD   = 5;
static const int16_t BAT_LEFT_AREA  = 130;

/*===========================================================================*/
/*                          CONSTRUCTOR                                       */
/*===========================================================================*/

BatteryScreenV1::BatteryScreenV1()
    : ScreenBase(ScreenId::BATTERY_V1)
    , _batteryBody(nullptr)
    , _batteryCap(nullptr)
    , _batteryFill(nullptr)
    , _lblSOC(nullptr)
    , _lblSOCBelow(nullptr)
    , _panelVoltage(nullptr)
    , _lblVoltageValue(nullptr)
    , _lblVoltageUnit(nullptr)
    , _panelCurrent(nullptr)
    , _lblCurrentValue(nullptr)
    , _lblCurrentUnit(nullptr)
    , _lblCurrentDir(nullptr)
    , _panelPower(nullptr)
    , _lblPowerValue(nullptr)
    , _lblPowerUnit(nullptr)
    , _statusDot(nullptr)
    , _lblStatus(nullptr)
    , _btnSwitchV2(nullptr)
    , _soc(72)
    , _voltage(54.2f)
    , _current(-12.5f)
    , _power(677.5f)
    , _temperature(32.0f)
    , _lastUpdate(0)
{
}

/*===========================================================================*/
/*                          SCREEN CREATION                                   */
/*===========================================================================*/

bool BatteryScreenV1::create() {
    /* Create screen with black background */
    _screen = lv_obj_create(nullptr);
    if (!_screen) return false;
    
    lv_obj_set_size(_screen, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_color(_screen, COL_BG, 0);
    lv_obj_set_style_bg_opa(_screen, LV_OPA_COVER, 0);
    lv_obj_clear_flag(_screen, LV_OBJ_FLAG_SCROLLABLE);
    
    /* ---- Top bar ---- */
    lv_obj_t* topBar = lv_obj_create(_screen);
    lv_obj_set_size(topBar, 480, 28);
    lv_obj_set_pos(topBar, 0, 0);
    lv_obj_set_style_bg_color(topBar, lv_color_hex(0x0D0D0D), 0);
    lv_obj_set_style_bg_opa(topBar, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(topBar, 0, 0);
    lv_obj_set_style_radius(topBar, 0, 0);
    lv_obj_set_style_pad_all(topBar, 0, 0);
    lv_obj_clear_flag(topBar, LV_OBJ_FLAG_SCROLLABLE);
    
    /* Title */
    lv_obj_t* lblTitle = lv_label_create(topBar);
    lv_label_set_text(lblTitle, LV_SYMBOL_CHARGE " BATTERY MONITOR");
    lv_obj_set_style_text_color(lblTitle, COL_CYAN, 0);
    lv_obj_set_style_text_font(lblTitle, &lv_font_montserrat_14, 0);
    lv_obj_align(lblTitle, LV_ALIGN_LEFT_MID, 10, 0);
    
    /* V1/V2 switch button */
    _btnSwitchV2 = lv_btn_create(topBar);
    lv_obj_set_size(_btnSwitchV2, 60, 22);
    lv_obj_align(_btnSwitchV2, LV_ALIGN_RIGHT_MID, -8, 0);
    lv_obj_set_style_bg_color(_btnSwitchV2, lv_color_hex(0x2A2A2A), 0);
    lv_obj_set_style_bg_color(_btnSwitchV2, lv_color_hex(0x404040), LV_STATE_PRESSED);
    lv_obj_set_style_radius(_btnSwitchV2, 4, 0);
    lv_obj_set_style_border_width(_btnSwitchV2, 1, 0);
    lv_obj_set_style_border_color(_btnSwitchV2, COL_CYAN, 0);
    lv_obj_set_style_shadow_width(_btnSwitchV2, 0, 0);
    lv_obj_set_style_pad_all(_btnSwitchV2, 0, 0);
    lv_obj_add_event_cb(_btnSwitchV2, onSwitchV2Click, LV_EVENT_CLICKED, this);
    
    lv_obj_t* lblSwitch = lv_label_create(_btnSwitchV2);
    lv_label_set_text(lblSwitch, "V2 " LV_SYMBOL_RIGHT);
    lv_obj_set_style_text_color(lblSwitch, COL_CYAN, 0);
    lv_obj_center(lblSwitch);
    
    /* ---- Main content area (below top bar) ---- */
    lv_obj_t* content = lv_obj_create(_screen);
    lv_obj_set_size(content, 480, 244);
    lv_obj_set_pos(content, 0, 28);
    lv_obj_set_style_bg_opa(content, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(content, 0, 0);
    lv_obj_set_style_pad_all(content, 0, 0);
    lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);
    
    /* Create sub-elements */
    createBatteryIcon(content);
    createParameterPanels(content);
    createStatusArea(content);
    
    /* Initial data update */
    updateBatteryFill();
    updateParameterValues();
    
    Serial.println("[BatteryV1] Created");
    return true;
}

/*===========================================================================*/
/*                     BATTERY ICON (LEFT SIDE)                               */
/*===========================================================================*/

void BatteryScreenV1::createBatteryIcon(lv_obj_t* parent) {
    /* Center of left area: x = BAT_LEFT_AREA/2, y = center of content */
    int16_t centerX = BAT_LEFT_AREA / 2;
    int16_t centerY = 105;  /* approx center of 244px content */
    
    /* Battery cap (top nub) */
    _batteryCap = lv_obj_create(parent);
    lv_obj_set_size(_batteryCap, BAT_CAP_W, BAT_CAP_H);
    lv_obj_set_pos(_batteryCap, centerX - BAT_CAP_W / 2, centerY - BAT_BODY_H / 2 - BAT_CAP_H + 2);
    lv_obj_set_style_bg_color(_batteryCap, COL_GREEN, 0);
    lv_obj_set_style_bg_opa(_batteryCap, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(_batteryCap, 0, 0);
    lv_obj_set_style_radius(_batteryCap, 3, 0);
    lv_obj_clear_flag(_batteryCap, LV_OBJ_FLAG_SCROLLABLE);
    
    /* Battery body outline */
    _batteryBody = lv_obj_create(parent);
    lv_obj_set_size(_batteryBody, BAT_BODY_W, BAT_BODY_H);
    lv_obj_set_pos(_batteryBody, centerX - BAT_BODY_W / 2, centerY - BAT_BODY_H / 2);
    lv_obj_set_style_bg_color(_batteryBody, lv_color_hex(0x111111), 0);
    lv_obj_set_style_bg_opa(_batteryBody, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(_batteryBody, 2, 0);
    lv_obj_set_style_border_color(_batteryBody, COL_GREEN, 0);
    lv_obj_set_style_radius(_batteryBody, 6, 0);
    lv_obj_set_style_pad_all(_batteryBody, BAT_FILL_PAD, 0);
    lv_obj_clear_flag(_batteryBody, LV_OBJ_FLAG_SCROLLABLE);
    
    /* Battery fill (colored bar inside body, grows from bottom) */
    _batteryFill = lv_obj_create(_batteryBody);
    lv_obj_set_style_bg_opa(_batteryFill, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(_batteryFill, 0, 0);
    lv_obj_set_style_radius(_batteryFill, 3, 0);
    lv_obj_clear_flag(_batteryFill, LV_OBJ_FLAG_SCROLLABLE);
    /* Size/position set in updateBatteryFill() */
    
    /* SOC label inside battery */
    _lblSOC = lv_label_create(_batteryBody);
    lv_obj_set_style_text_font(_lblSOC, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(_lblSOC, COL_WHITE, 0);
    lv_obj_center(_lblSOC);
    lv_label_set_text_fmt(_lblSOC, "%d%%", _soc);
    
    /* SOC label below battery */
    _lblSOCBelow = lv_label_create(parent);
    lv_obj_set_style_text_font(_lblSOCBelow, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(_lblSOCBelow, COL_GREY, 0);
    lv_obj_set_pos(_lblSOCBelow, centerX - 10, centerY + BAT_BODY_H / 2 + 6);
    lv_label_set_text(_lblSOCBelow, "SOC");
}

/*===========================================================================*/
/*                    PARAMETER PANELS (RIGHT SIDE)                           */
/*===========================================================================*/

void BatteryScreenV1::createParameterPanels(lv_obj_t* parent) {
    int16_t panelX = BAT_LEFT_AREA + 5;
    int16_t panelW = 480 - BAT_LEFT_AREA - 15;
    int16_t panelH = 58;
    int16_t gap = 6;
    int16_t startY = 5;
    
    /* ---- Voltage Panel ---- */
    _panelVoltage = lv_obj_create(parent);
    lv_obj_set_size(_panelVoltage, panelW, panelH);
    lv_obj_set_pos(_panelVoltage, panelX, startY);
    lv_obj_set_style_bg_color(_panelVoltage, COL_PANEL, 0);
    lv_obj_set_style_bg_opa(_panelVoltage, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(_panelVoltage, 1, 0);
    lv_obj_set_style_border_color(_panelVoltage, COL_PANEL_BRD, 0);
    lv_obj_set_style_radius(_panelVoltage, 8, 0);
    lv_obj_set_style_pad_left(_panelVoltage, 12, 0);
    lv_obj_set_style_pad_right(_panelVoltage, 12, 0);
    lv_obj_clear_flag(_panelVoltage, LV_OBJ_FLAG_SCROLLABLE);
    
    /* Voltage icon + label */
    lv_obj_t* lblVoltIcon = lv_label_create(_panelVoltage);
    lv_label_set_text(lblVoltIcon, LV_SYMBOL_CHARGE);
    lv_obj_set_style_text_color(lblVoltIcon, COL_CYAN, 0);
    lv_obj_set_style_text_font(lblVoltIcon, &lv_font_montserrat_16, 0);
    lv_obj_align(lblVoltIcon, LV_ALIGN_LEFT_MID, 0, -12);
    
    lv_obj_t* lblVoltName = lv_label_create(_panelVoltage);
    lv_label_set_text(lblVoltName, "Voltage");
    lv_obj_set_style_text_color(lblVoltName, COL_GREY, 0);
    lv_obj_set_style_text_font(lblVoltName, &lv_font_montserrat_14, 0);
    lv_obj_align(lblVoltName, LV_ALIGN_LEFT_MID, 22, -12);
    
    /* Voltage value */
    _lblVoltageValue = lv_label_create(_panelVoltage);
    lv_obj_set_style_text_font(_lblVoltageValue, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(_lblVoltageValue, COL_CYAN, 0);
    lv_obj_align(_lblVoltageValue, LV_ALIGN_LEFT_MID, 0, 12);
    
    /* Voltage unit */
    _lblVoltageUnit = lv_label_create(_panelVoltage);
    lv_label_set_text(_lblVoltageUnit, "V");
    lv_obj_set_style_text_font(_lblVoltageUnit, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(_lblVoltageUnit, COL_CYAN, 0);
    lv_obj_align(_lblVoltageUnit, LV_ALIGN_RIGHT_MID, 0, 12);
    
    /* ---- Current Panel ---- */
    _panelCurrent = lv_obj_create(parent);
    lv_obj_set_size(_panelCurrent, panelW, panelH);
    lv_obj_set_pos(_panelCurrent, panelX, startY + panelH + gap);
    lv_obj_set_style_bg_color(_panelCurrent, COL_PANEL, 0);
    lv_obj_set_style_bg_opa(_panelCurrent, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(_panelCurrent, 1, 0);
    lv_obj_set_style_border_color(_panelCurrent, COL_PANEL_BRD, 0);
    lv_obj_set_style_radius(_panelCurrent, 8, 0);
    lv_obj_set_style_pad_left(_panelCurrent, 12, 0);
    lv_obj_set_style_pad_right(_panelCurrent, 12, 0);
    lv_obj_clear_flag(_panelCurrent, LV_OBJ_FLAG_SCROLLABLE);
    
    /* Current icon + label */
    lv_obj_t* lblCurIcon = lv_label_create(_panelCurrent);
    lv_label_set_text(lblCurIcon, LV_SYMBOL_LOOP);
    lv_obj_set_style_text_color(lblCurIcon, COL_GREEN, 0);
    lv_obj_set_style_text_font(lblCurIcon, &lv_font_montserrat_16, 0);
    lv_obj_align(lblCurIcon, LV_ALIGN_LEFT_MID, 0, -12);
    
    lv_obj_t* lblCurName = lv_label_create(_panelCurrent);
    lv_label_set_text(lblCurName, "Current");
    lv_obj_set_style_text_color(lblCurName, COL_GREY, 0);
    lv_obj_set_style_text_font(lblCurName, &lv_font_montserrat_14, 0);
    lv_obj_align(lblCurName, LV_ALIGN_LEFT_MID, 22, -12);
    
    /* Current direction indicator */
    _lblCurrentDir = lv_label_create(_panelCurrent);
    lv_obj_set_style_text_font(_lblCurrentDir, &lv_font_montserrat_14, 0);
    lv_obj_align(_lblCurrentDir, LV_ALIGN_RIGHT_MID, 0, -12);
    
    /* Current value */
    _lblCurrentValue = lv_label_create(_panelCurrent);
    lv_obj_set_style_text_font(_lblCurrentValue, &lv_font_montserrat_24, 0);
    lv_obj_align(_lblCurrentValue, LV_ALIGN_LEFT_MID, 0, 12);
    
    /* Current unit */
    _lblCurrentUnit = lv_label_create(_panelCurrent);
    lv_label_set_text(_lblCurrentUnit, "A");
    lv_obj_set_style_text_font(_lblCurrentUnit, &lv_font_montserrat_16, 0);
    lv_obj_align(_lblCurrentUnit, LV_ALIGN_RIGHT_MID, 0, 12);
    
    /* ---- Power Panel ---- */
    _panelPower = lv_obj_create(parent);
    lv_obj_set_size(_panelPower, panelW, panelH);
    lv_obj_set_pos(_panelPower, panelX, startY + 2 * (panelH + gap));
    lv_obj_set_style_bg_color(_panelPower, COL_PANEL, 0);
    lv_obj_set_style_bg_opa(_panelPower, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(_panelPower, 1, 0);
    lv_obj_set_style_border_color(_panelPower, COL_PANEL_BRD, 0);
    lv_obj_set_style_radius(_panelPower, 8, 0);
    lv_obj_set_style_pad_left(_panelPower, 12, 0);
    lv_obj_set_style_pad_right(_panelPower, 12, 0);
    lv_obj_clear_flag(_panelPower, LV_OBJ_FLAG_SCROLLABLE);
    
    /* Power icon + label */
    lv_obj_t* lblPwrIcon = lv_label_create(_panelPower);
    lv_label_set_text(lblPwrIcon, LV_SYMBOL_POWER);
    lv_obj_set_style_text_color(lblPwrIcon, COL_YELLOW, 0);
    lv_obj_set_style_text_font(lblPwrIcon, &lv_font_montserrat_16, 0);
    lv_obj_align(lblPwrIcon, LV_ALIGN_LEFT_MID, 0, -12);
    
    lv_obj_t* lblPwrName = lv_label_create(_panelPower);
    lv_label_set_text(lblPwrName, "Power");
    lv_obj_set_style_text_color(lblPwrName, COL_GREY, 0);
    lv_obj_set_style_text_font(lblPwrName, &lv_font_montserrat_14, 0);
    lv_obj_align(lblPwrName, LV_ALIGN_LEFT_MID, 22, -12);
    
    /* Power value */
    _lblPowerValue = lv_label_create(_panelPower);
    lv_obj_set_style_text_font(_lblPowerValue, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(_lblPowerValue, COL_YELLOW, 0);
    lv_obj_align(_lblPowerValue, LV_ALIGN_LEFT_MID, 0, 12);
    
    /* Power unit */
    _lblPowerUnit = lv_label_create(_panelPower);
    lv_label_set_text(_lblPowerUnit, "W");
    lv_obj_set_style_text_font(_lblPowerUnit, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(_lblPowerUnit, COL_YELLOW, 0);
    lv_obj_align(_lblPowerUnit, LV_ALIGN_RIGHT_MID, 0, 12);
}

/*===========================================================================*/
/*                        STATUS AREA                                         */
/*===========================================================================*/

void BatteryScreenV1::createStatusArea(lv_obj_t* parent) {
    int16_t panelX = BAT_LEFT_AREA + 5;
    int16_t panelW = 480 - BAT_LEFT_AREA - 15;
    int16_t statusY = 5 + 3 * (58 + 6);  /* Below 3 panels */
    
    /* Status container */
    lv_obj_t* statusPanel = lv_obj_create(parent);
    lv_obj_set_size(statusPanel, panelW, 46);
    lv_obj_set_pos(statusPanel, panelX, statusY);
    lv_obj_set_style_bg_color(statusPanel, COL_PANEL, 0);
    lv_obj_set_style_bg_opa(statusPanel, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(statusPanel, 1, 0);
    lv_obj_set_style_border_color(statusPanel, COL_PANEL_BRD, 0);
    lv_obj_set_style_radius(statusPanel, 8, 0);
    lv_obj_set_style_pad_left(statusPanel, 12, 0);
    lv_obj_clear_flag(statusPanel, LV_OBJ_FLAG_SCROLLABLE);
    
    /* Status LED dot */
    _statusDot = lv_obj_create(statusPanel);
    lv_obj_set_size(_statusDot, 10, 10);
    lv_obj_align(_statusDot, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_style_bg_color(_statusDot, COL_GREEN, 0);
    lv_obj_set_style_bg_opa(_statusDot, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(_statusDot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(_statusDot, 0, 0);
    lv_obj_set_style_shadow_width(_statusDot, 8, 0);
    lv_obj_set_style_shadow_color(_statusDot, COL_GREEN, 0);
    lv_obj_set_style_shadow_spread(_statusDot, 2, 0);
    lv_obj_clear_flag(_statusDot, LV_OBJ_FLAG_SCROLLABLE);
    
    /* Status label */
    _lblStatus = lv_label_create(statusPanel);
    lv_label_set_text(_lblStatus, "Discharging");
    lv_obj_set_style_text_color(_lblStatus, COL_WHITE, 0);
    lv_obj_set_style_text_font(_lblStatus, &lv_font_montserrat_16, 0);
    lv_obj_align(_lblStatus, LV_ALIGN_LEFT_MID, 18, 0);
    
    /* Temperature on the right */
    lv_obj_t* lblTemp = lv_label_create(statusPanel);
    lv_label_set_text_fmt(lblTemp, "%.1f" "\xC2\xB0" "C", _temperature);
    lv_obj_set_style_text_color(lblTemp, COL_GREY, 0);
    lv_obj_set_style_text_font(lblTemp, &lv_font_montserrat_14, 0);
    lv_obj_align(lblTemp, LV_ALIGN_RIGHT_MID, -12, 0);
}

/*===========================================================================*/
/*                       UPDATE FUNCTIONS                                     */
/*===========================================================================*/

void BatteryScreenV1::updateBatteryFill() {
    if (!_batteryFill || !_batteryBody) return;
    
    /* Calculate fill height based on SOC */
    int16_t innerW = BAT_BODY_W - 2 * 2 - 2 * BAT_FILL_PAD;  /* body - border - padding */
    int16_t innerH = BAT_BODY_H - 2 * 2 - 2 * BAT_FILL_PAD;
    int16_t fillH = (int16_t)((int32_t)innerH * _soc / 100);
    
    if (fillH < 4) fillH = 4;  /* Minimum visible */
    
    /* Set fill size and position (anchored to bottom) */
    lv_obj_set_size(_batteryFill, innerW, fillH);
    lv_obj_align(_batteryFill, LV_ALIGN_BOTTOM_MID, 0, 0);
    
    /* Set fill color based on SOC */
    lv_color_t fillColor = getSOCColor(_soc);
    lv_obj_set_style_bg_color(_batteryFill, fillColor, 0);
    
    /* Update border color of battery body to match */
    lv_obj_set_style_border_color(_batteryBody, fillColor, 0);
    
    /* Update cap color */
    if (_batteryCap) {
        lv_obj_set_style_bg_color(_batteryCap, fillColor, 0);
    }
    
    /* Update SOC text */
    if (_lblSOC) {
        lv_label_set_text_fmt(_lblSOC, "%d%%", _soc);
        lv_obj_center(_lblSOC);
    }
}

void BatteryScreenV1::updateParameterValues() {
    /* Voltage */
    if (_lblVoltageValue) {
        lv_label_set_text_fmt(_lblVoltageValue, "%.1f", _voltage);
    }
    
    /* Current */
    if (_lblCurrentValue) {
        float absCurrent = _current < 0 ? -_current : _current;
        lv_label_set_text_fmt(_lblCurrentValue, "%.1f", absCurrent);
        
        /* Color and direction based on sign */
        lv_color_t curColor;
        const char* dirText;
        if (_current > 0.1f) {
            curColor = COL_GREEN;
            dirText = LV_SYMBOL_UP " CHG";
        } else if (_current < -0.1f) {
            curColor = COL_RED;
            dirText = LV_SYMBOL_DOWN " DSG";
        } else {
            curColor = COL_GREY;
            dirText = "IDLE";
        }
        
        lv_obj_set_style_text_color(_lblCurrentValue, curColor, 0);
        lv_obj_set_style_text_color(_lblCurrentUnit, curColor, 0);
        
        if (_lblCurrentDir) {
            lv_label_set_text(_lblCurrentDir, dirText);
            lv_obj_set_style_text_color(_lblCurrentDir, curColor, 0);
        }
    }
    
    /* Power */
    if (_lblPowerValue) {
        lv_label_set_text_fmt(_lblPowerValue, "%.1f", _power);
    }
}

lv_color_t BatteryScreenV1::getSOCColor(uint8_t soc) {
    if (soc <= 20) return COL_RED;
    if (soc <= 50) return COL_ORANGE;
    if (soc <= 80) return COL_YELLOW;
    return COL_GREEN;
}

/*===========================================================================*/
/*                         LIFECYCLE                                          */
/*===========================================================================*/

void BatteryScreenV1::onEnter() {
    ScreenBase::onEnter();
    _lastUpdate = 0;
}

void BatteryScreenV1::onExit() {
    ScreenBase::onExit();
}

void BatteryScreenV1::update() {
    uint32_t now = millis();
    if (now - _lastUpdate > 1000) {
        _lastUpdate = now;
        
        /* Demo: cycle SOC slowly for visual testing */
        /* In production, data would come from BMS service */
        static int8_t dir = -1;
        _soc += dir;
        if (_soc <= 5) dir = 1;
        if (_soc >= 100) dir = -1;
        
        /* Recalculate power from voltage * current */
        _power = _voltage * (_current < 0 ? -_current : _current);
        
        updateBatteryFill();
        updateParameterValues();
    }
}

/*===========================================================================*/
/*                        DATA SETTERS                                        */
/*===========================================================================*/

void BatteryScreenV1::setSOC(uint8_t percent) {
    if (percent > 100) percent = 100;
    _soc = percent;
    if (_active) updateBatteryFill();
}

void BatteryScreenV1::setVoltage(float volts) {
    _voltage = volts;
    if (_active) updateParameterValues();
}

void BatteryScreenV1::setCurrent(float amps) {
    _current = amps;
    if (_active) updateParameterValues();
}

void BatteryScreenV1::setPower(float watts) {
    _power = watts;
    if (_active) updateParameterValues();
}

void BatteryScreenV1::setStatus(const char* status, uint32_t color) {
    if (_lblStatus) {
        lv_label_set_text(_lblStatus, status);
    }
    if (_statusDot) {
        lv_color_t c = lv_color_hex(color);
        lv_obj_set_style_bg_color(_statusDot, c, 0);
        lv_obj_set_style_shadow_color(_statusDot, c, 0);
    }
}

void BatteryScreenV1::setTemperature(float tempC) {
    _temperature = tempC;
}

/*===========================================================================*/
/*                        EVENT HANDLERS                                      */
/*===========================================================================*/

void BatteryScreenV1::onSwitchV2Click(lv_event_t* e) {
    SCREEN_MGR.navigateTo(ScreenId::BATTERY_V2, ScreenTransition::SLIDE_LEFT);
}

} // namespace UI
