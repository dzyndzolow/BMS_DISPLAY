/**
 * @file battery_screen_v2.cpp
 * @brief Battery Monitor Screen V2 implementation
 * 
 * Circular arc gauge SOC indicator with voltage, current, power and status.
 * Black background, colored accents. The arc uses a 270-degree sweep.
 * 
 * @version 1.0.0
 */

#include "battery_screen_v2.h"
#include "screen_manager.h"
#include <Arduino.h>

namespace UI {

/*===========================================================================*/
/*                          COLOR PALETTE                                     */
/*===========================================================================*/

static const lv_color_t COL2_BG         = lv_color_hex(0x000000);
static const lv_color_t COL2_PANEL      = lv_color_hex(0x1A1A1A);
static const lv_color_t COL2_PANEL_BRD  = lv_color_hex(0x2A2A2A);
static const lv_color_t COL2_CYAN       = lv_color_hex(0x00E5FF);
static const lv_color_t COL2_GREEN      = lv_color_hex(0x00E676);
static const lv_color_t COL2_RED        = lv_color_hex(0xFF5252);
static const lv_color_t COL2_YELLOW     = lv_color_hex(0xFFD740);
static const lv_color_t COL2_ORANGE     = lv_color_hex(0xFF9100);
static const lv_color_t COL2_WHITE      = lv_color_hex(0xFFFFFF);
static const lv_color_t COL2_GREY       = lv_color_hex(0x888888);
static const lv_color_t COL2_ARC_BG     = lv_color_hex(0x1A1A1A);

/* Layout constants */
static const int16_t ARC_AREA_W    = 220;   /* Left area for arc gauge */
static const int16_t ARC_SIZE      = 190;   /* Arc diameter */
static const int16_t ARC_WIDTH     = 14;    /* Arc line width */

/*===========================================================================*/
/*                          CONSTRUCTOR                                       */
/*===========================================================================*/

BatteryScreenV2::BatteryScreenV2()
    : ScreenBase(ScreenId::BATTERY_V2)
    , _arcGauge(nullptr)
    , _lblSOCValue(nullptr)
    , _lblSOCUnit(nullptr)
    , _lblSOCLabel(nullptr)
    , _panelVoltage(nullptr)
    , _lblVoltageIcon(nullptr)
    , _lblVoltageName(nullptr)
    , _lblVoltageValue(nullptr)
    , _panelCurrent(nullptr)
    , _lblCurrentIcon(nullptr)
    , _lblCurrentName(nullptr)
    , _lblCurrentValue(nullptr)
    , _lblCurrentDir(nullptr)
    , _panelPower(nullptr)
    , _lblPowerIcon(nullptr)
    , _lblPowerName(nullptr)
    , _lblPowerValue(nullptr)
    , _statusDot(nullptr)
    , _lblStatus(nullptr)
    , _sliderBrightness(nullptr)
    , _lblBrightnessIcon(nullptr)
    , _btnSwitchV1(nullptr)
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

bool BatteryScreenV2::create() {
    _screen = lv_obj_create(nullptr);
    if (!_screen) return false;
    
    lv_obj_set_size(_screen, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_color(_screen, COL2_BG, 0);
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
    lv_obj_set_style_text_color(lblTitle, COL2_CYAN, 0);
    lv_obj_set_style_text_font(lblTitle, &lv_font_montserrat_14, 0);
    lv_obj_align(lblTitle, LV_ALIGN_LEFT_MID, 10, 0);
    
    /* V1 switch button */
    _btnSwitchV1 = lv_btn_create(topBar);
    lv_obj_set_size(_btnSwitchV1, 60, 22);
    lv_obj_align(_btnSwitchV1, LV_ALIGN_RIGHT_MID, -8, 0);
    lv_obj_set_style_bg_color(_btnSwitchV1, lv_color_hex(0x2A2A2A), 0);
    lv_obj_set_style_bg_color(_btnSwitchV1, lv_color_hex(0x404040), LV_STATE_PRESSED);
    lv_obj_set_style_radius(_btnSwitchV1, 4, 0);
    lv_obj_set_style_border_width(_btnSwitchV1, 1, 0);
    lv_obj_set_style_border_color(_btnSwitchV1, COL2_CYAN, 0);
    lv_obj_set_style_shadow_width(_btnSwitchV1, 0, 0);
    lv_obj_set_style_pad_all(_btnSwitchV1, 0, 0);
    lv_obj_add_event_cb(_btnSwitchV1, onSwitchV1Click, LV_EVENT_CLICKED, this);
    
    lv_obj_t* lblSwitch = lv_label_create(_btnSwitchV1);
    lv_label_set_text(lblSwitch, LV_SYMBOL_LEFT " V1");
    lv_obj_set_style_text_color(lblSwitch, COL2_CYAN, 0);
    lv_obj_center(lblSwitch);
    
    /* ---- Main content area ---- */
    lv_obj_t* content = lv_obj_create(_screen);
    lv_obj_set_size(content, 480, 244);
    lv_obj_set_pos(content, 0, 28);
    lv_obj_set_style_bg_opa(content, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(content, 0, 0);
    lv_obj_set_style_pad_all(content, 0, 0);
    lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);
    
    createArcGauge(content);
    createParameterPanels(content);
    createStatusArea(content);
    
    updateArcGauge();
    updateParameterValues();
    
    Serial.println("[BatteryV2] Created");
    return true;
}

/*===========================================================================*/
/*                     ARC GAUGE (LEFT SIDE)                                  */
/*===========================================================================*/

void BatteryScreenV2::createArcGauge(lv_obj_t* parent) {
    int16_t centerX = ARC_AREA_W / 2;
    int16_t centerY = 122;  /* Center of 244px */
    
    /* Create arc */
    _arcGauge = lv_arc_create(parent);
    lv_obj_set_size(_arcGauge, ARC_SIZE, ARC_SIZE);
    lv_obj_set_pos(_arcGauge, centerX - ARC_SIZE / 2, centerY - ARC_SIZE / 2);
    
    /* Arc appearance */
    lv_arc_set_rotation(_arcGauge, 135);        /* Start at bottom-left */
    lv_arc_set_bg_angles(_arcGauge, 0, 270);    /* 270 degree sweep */
    lv_arc_set_range(_arcGauge, 0, 100);
    lv_arc_set_value(_arcGauge, _soc);
    
    /* Background arc (track) */
    lv_obj_set_style_arc_color(_arcGauge, COL2_ARC_BG, LV_PART_MAIN);
    lv_obj_set_style_arc_width(_arcGauge, ARC_WIDTH, LV_PART_MAIN);
    lv_obj_set_style_arc_rounded(_arcGauge, true, LV_PART_MAIN);
    
    /* Active arc (indicator) */
    lv_obj_set_style_arc_color(_arcGauge, COL2_GREEN, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(_arcGauge, ARC_WIDTH, LV_PART_INDICATOR);
    lv_obj_set_style_arc_rounded(_arcGauge, true, LV_PART_INDICATOR);
    
    /* Hide the knob */
    lv_obj_set_style_bg_opa(_arcGauge, LV_OPA_TRANSP, LV_PART_KNOB);
    lv_obj_set_style_pad_all(_arcGauge, 0, LV_PART_KNOB);
    
    /* Remove interactivity (display only) */
    lv_obj_clear_flag(_arcGauge, LV_OBJ_FLAG_CLICKABLE);
    
    /* SOC percentage - large text in center of arc */
    _lblSOCValue = lv_label_create(parent);
    lv_obj_set_style_text_font(_lblSOCValue, &lv_font_montserrat_36, 0);
    lv_obj_set_style_text_color(_lblSOCValue, COL2_WHITE, 0);
    lv_label_set_text_fmt(_lblSOCValue, "%d", _soc);
    lv_obj_set_pos(_lblSOCValue, centerX - 25, centerY - 24);
    
    /* "%" unit */
    _lblSOCUnit = lv_label_create(parent);
    lv_obj_set_style_text_font(_lblSOCUnit, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(_lblSOCUnit, COL2_GREY, 0);
    lv_label_set_text(_lblSOCUnit, "%");
    lv_obj_set_pos(_lblSOCUnit, centerX + 25, centerY - 14);
    
    /* "SOC" label below percentage */
    _lblSOCLabel = lv_label_create(parent);
    lv_obj_set_style_text_font(_lblSOCLabel, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(_lblSOCLabel, COL2_GREY, 0);
    lv_label_set_text(_lblSOCLabel, "SOC");
    lv_obj_set_pos(_lblSOCLabel, centerX - 12, centerY + 16);
}

/*===========================================================================*/
/*                    PARAMETER PANELS (RIGHT SIDE)                           */
/*===========================================================================*/

void BatteryScreenV2::createParameterPanels(lv_obj_t* parent) {
    int16_t panelX = ARC_AREA_W + 5;
    int16_t panelW = 480 - ARC_AREA_W - 15;
    int16_t panelH = 52;
    int16_t gap = 5;
    int16_t startY = 5;
    
    /* ---- Voltage Panel ---- */
    _panelVoltage = lv_obj_create(parent);
    lv_obj_set_size(_panelVoltage, panelW, panelH);
    lv_obj_set_pos(_panelVoltage, panelX, startY);
    lv_obj_set_style_bg_color(_panelVoltage, COL2_PANEL, 0);
    lv_obj_set_style_bg_opa(_panelVoltage, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(_panelVoltage, 1, 0);
    lv_obj_set_style_border_color(_panelVoltage, COL2_PANEL_BRD, 0);
    lv_obj_set_style_border_side(_panelVoltage, LV_BORDER_SIDE_LEFT, 0);
    lv_obj_set_style_border_color(_panelVoltage, COL2_CYAN, 0);
    lv_obj_set_style_border_width(_panelVoltage, 3, 0);
    lv_obj_set_style_radius(_panelVoltage, 6, 0);
    lv_obj_set_style_pad_left(_panelVoltage, 14, 0);
    lv_obj_set_style_pad_right(_panelVoltage, 10, 0);
    lv_obj_clear_flag(_panelVoltage, LV_OBJ_FLAG_SCROLLABLE);
    
    _lblVoltageIcon = lv_label_create(_panelVoltage);
    lv_label_set_text(_lblVoltageIcon, LV_SYMBOL_CHARGE);
    lv_obj_set_style_text_color(_lblVoltageIcon, COL2_CYAN, 0);
    lv_obj_align(_lblVoltageIcon, LV_ALIGN_LEFT_MID, 0, -10);
    
    _lblVoltageName = lv_label_create(_panelVoltage);
    lv_label_set_text(_lblVoltageName, "Voltage");
    lv_obj_set_style_text_color(_lblVoltageName, COL2_GREY, 0);
    lv_obj_set_style_text_font(_lblVoltageName, &lv_font_montserrat_14, 0);
    lv_obj_align(_lblVoltageName, LV_ALIGN_LEFT_MID, 20, -10);
    
    _lblVoltageValue = lv_label_create(_panelVoltage);
    lv_obj_set_style_text_font(_lblVoltageValue, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(_lblVoltageValue, COL2_CYAN, 0);
    lv_obj_align(_lblVoltageValue, LV_ALIGN_LEFT_MID, 0, 10);
    
    lv_obj_t* lblVUnit = lv_label_create(_panelVoltage);
    lv_label_set_text(lblVUnit, "V");
    lv_obj_set_style_text_font(lblVUnit, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(lblVUnit, COL2_CYAN, 0);
    lv_obj_align(lblVUnit, LV_ALIGN_RIGHT_MID, 0, 10);
    
    /* ---- Current Panel ---- */
    _panelCurrent = lv_obj_create(parent);
    lv_obj_set_size(_panelCurrent, panelW, panelH);
    lv_obj_set_pos(_panelCurrent, panelX, startY + panelH + gap);
    lv_obj_set_style_bg_color(_panelCurrent, COL2_PANEL, 0);
    lv_obj_set_style_bg_opa(_panelCurrent, LV_OPA_COVER, 0);
    lv_obj_set_style_border_side(_panelCurrent, LV_BORDER_SIDE_LEFT, 0);
    lv_obj_set_style_border_color(_panelCurrent, COL2_GREEN, 0);
    lv_obj_set_style_border_width(_panelCurrent, 3, 0);
    lv_obj_set_style_radius(_panelCurrent, 6, 0);
    lv_obj_set_style_pad_left(_panelCurrent, 14, 0);
    lv_obj_set_style_pad_right(_panelCurrent, 10, 0);
    lv_obj_clear_flag(_panelCurrent, LV_OBJ_FLAG_SCROLLABLE);
    
    _lblCurrentIcon = lv_label_create(_panelCurrent);
    lv_label_set_text(_lblCurrentIcon, LV_SYMBOL_LOOP);
    lv_obj_set_style_text_color(_lblCurrentIcon, COL2_GREEN, 0);
    lv_obj_align(_lblCurrentIcon, LV_ALIGN_LEFT_MID, 0, -10);
    
    _lblCurrentName = lv_label_create(_panelCurrent);
    lv_label_set_text(_lblCurrentName, "Current");
    lv_obj_set_style_text_color(_lblCurrentName, COL2_GREY, 0);
    lv_obj_set_style_text_font(_lblCurrentName, &lv_font_montserrat_14, 0);
    lv_obj_align(_lblCurrentName, LV_ALIGN_LEFT_MID, 20, -10);
    
    _lblCurrentDir = lv_label_create(_panelCurrent);
    lv_obj_set_style_text_font(_lblCurrentDir, &lv_font_montserrat_14, 0);
    lv_obj_align(_lblCurrentDir, LV_ALIGN_RIGHT_MID, 0, -10);
    
    _lblCurrentValue = lv_label_create(_panelCurrent);
    lv_obj_set_style_text_font(_lblCurrentValue, &lv_font_montserrat_20, 0);
    lv_obj_align(_lblCurrentValue, LV_ALIGN_LEFT_MID, 0, 10);
    
    lv_obj_t* lblAUnit = lv_label_create(_panelCurrent);
    lv_label_set_text(lblAUnit, "A");
    lv_obj_set_style_text_font(lblAUnit, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(lblAUnit, COL2_GREEN, 0);
    lv_obj_align(lblAUnit, LV_ALIGN_RIGHT_MID, 0, 10);
    
    /* ---- Power Panel ---- */
    _panelPower = lv_obj_create(parent);
    lv_obj_set_size(_panelPower, panelW, panelH);
    lv_obj_set_pos(_panelPower, panelX, startY + 2 * (panelH + gap));
    lv_obj_set_style_bg_color(_panelPower, COL2_PANEL, 0);
    lv_obj_set_style_bg_opa(_panelPower, LV_OPA_COVER, 0);
    lv_obj_set_style_border_side(_panelPower, LV_BORDER_SIDE_LEFT, 0);
    lv_obj_set_style_border_color(_panelPower, COL2_YELLOW, 0);
    lv_obj_set_style_border_width(_panelPower, 3, 0);
    lv_obj_set_style_radius(_panelPower, 6, 0);
    lv_obj_set_style_pad_left(_panelPower, 14, 0);
    lv_obj_set_style_pad_right(_panelPower, 10, 0);
    lv_obj_clear_flag(_panelPower, LV_OBJ_FLAG_SCROLLABLE);
    
    _lblPowerIcon = lv_label_create(_panelPower);
    lv_label_set_text(_lblPowerIcon, LV_SYMBOL_POWER);
    lv_obj_set_style_text_color(_lblPowerIcon, COL2_YELLOW, 0);
    lv_obj_align(_lblPowerIcon, LV_ALIGN_LEFT_MID, 0, -10);
    
    _lblPowerName = lv_label_create(_panelPower);
    lv_label_set_text(_lblPowerName, "Power");
    lv_obj_set_style_text_color(_lblPowerName, COL2_GREY, 0);
    lv_obj_set_style_text_font(_lblPowerName, &lv_font_montserrat_14, 0);
    lv_obj_align(_lblPowerName, LV_ALIGN_LEFT_MID, 20, -10);
    
    _lblPowerValue = lv_label_create(_panelPower);
    lv_obj_set_style_text_font(_lblPowerValue, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(_lblPowerValue, COL2_YELLOW, 0);
    lv_obj_align(_lblPowerValue, LV_ALIGN_LEFT_MID, 0, 10);
    
    lv_obj_t* lblWUnit = lv_label_create(_panelPower);
    lv_label_set_text(lblWUnit, "W");
    lv_obj_set_style_text_font(lblWUnit, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(lblWUnit, COL2_YELLOW, 0);
    lv_obj_align(lblWUnit, LV_ALIGN_RIGHT_MID, 0, 10);
}

/*===========================================================================*/
/*                        STATUS AREA                                         */
/*===========================================================================*/

void BatteryScreenV2::createStatusArea(lv_obj_t* parent) {
    int16_t panelX = ARC_AREA_W + 5;
    int16_t panelW = 480 - ARC_AREA_W - 15;
    int16_t statusY = 5 + 3 * (52 + 5);
    
    lv_obj_t* statusPanel = lv_obj_create(parent);
    lv_obj_set_size(statusPanel, panelW, 68);
    lv_obj_set_pos(statusPanel, panelX, statusY);
    lv_obj_set_style_bg_color(statusPanel, COL2_PANEL, 0);
    lv_obj_set_style_bg_opa(statusPanel, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(statusPanel, 1, 0);
    lv_obj_set_style_border_color(statusPanel, COL2_PANEL_BRD, 0);
    lv_obj_set_style_radius(statusPanel, 6, 0);
    lv_obj_set_style_pad_left(statusPanel, 14, 0);
    lv_obj_clear_flag(statusPanel, LV_OBJ_FLAG_SCROLLABLE);
    
    /* Status LED dot */
    _statusDot = lv_obj_create(statusPanel);
    lv_obj_set_size(_statusDot, 10, 10);
    lv_obj_align(_statusDot, LV_ALIGN_LEFT_MID, 0, -8);
    lv_obj_set_style_bg_color(_statusDot, COL2_GREEN, 0);
    lv_obj_set_style_bg_opa(_statusDot, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(_statusDot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(_statusDot, 0, 0);
    lv_obj_set_style_shadow_width(_statusDot, 8, 0);
    lv_obj_set_style_shadow_color(_statusDot, COL2_GREEN, 0);
    lv_obj_set_style_shadow_spread(_statusDot, 2, 0);
    lv_obj_clear_flag(_statusDot, LV_OBJ_FLAG_SCROLLABLE);
    
    /* Status label */
    _lblStatus = lv_label_create(statusPanel);
    lv_label_set_text(_lblStatus, "Discharging");
    lv_obj_set_style_text_color(_lblStatus, COL2_WHITE, 0);
    lv_obj_set_style_text_font(_lblStatus, &lv_font_montserrat_16, 0);
    lv_obj_align(_lblStatus, LV_ALIGN_LEFT_MID, 18, -8);
    
    /* Brightness control */
    _lblBrightnessIcon = lv_label_create(statusPanel);
    lv_label_set_text(_lblBrightnessIcon, LV_SYMBOL_IMAGE);  /* sun-like icon */
    lv_obj_set_style_text_color(_lblBrightnessIcon, COL2_GREY, 0);
    lv_obj_align(_lblBrightnessIcon, LV_ALIGN_LEFT_MID, 0, 14);
    
    _sliderBrightness = lv_slider_create(statusPanel);
    lv_obj_set_size(_sliderBrightness, 150, 8);
    lv_obj_align(_sliderBrightness, LV_ALIGN_LEFT_MID, 22, 14);
    lv_slider_set_range(_sliderBrightness, 10, 100);
    lv_slider_set_value(_sliderBrightness, 100, LV_ANIM_OFF);
    
    /* Slider track (background) */
    lv_obj_set_style_bg_color(_sliderBrightness, lv_color_hex(0x333333), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(_sliderBrightness, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_radius(_sliderBrightness, 4, LV_PART_MAIN);
    
    /* Slider indicator (filled part) */
    lv_obj_set_style_bg_color(_sliderBrightness, COL2_CYAN, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(_sliderBrightness, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_set_style_radius(_sliderBrightness, 4, LV_PART_INDICATOR);
    
    /* Slider knob */
    lv_obj_set_style_bg_color(_sliderBrightness, COL2_WHITE, LV_PART_KNOB);
    lv_obj_set_style_bg_opa(_sliderBrightness, LV_OPA_COVER, LV_PART_KNOB);
    lv_obj_set_style_pad_all(_sliderBrightness, 3, LV_PART_KNOB);
    
    lv_obj_add_event_cb(_sliderBrightness, onBrightnessChange, LV_EVENT_VALUE_CHANGED, nullptr);
}

/*===========================================================================*/
/*                       UPDATE FUNCTIONS                                     */
/*===========================================================================*/

void BatteryScreenV2::updateArcGauge() {
    if (!_arcGauge) return;
    
    lv_arc_set_value(_arcGauge, _soc);
    
    /* Update arc color based on SOC */
    lv_color_t arcColor = getSOCColor(_soc);
    lv_obj_set_style_arc_color(_arcGauge, arcColor, LV_PART_INDICATOR);
    
    /* Update SOC text */
    if (_lblSOCValue) {
        lv_label_set_text_fmt(_lblSOCValue, "%d", _soc);
    }
}

void BatteryScreenV2::updateParameterValues() {
    /* Voltage */
    if (_lblVoltageValue) {
        lv_label_set_text_fmt(_lblVoltageValue, "%.1f", _voltage);
    }
    
    /* Current */
    if (_lblCurrentValue) {
        float absCurrent = _current < 0 ? -_current : _current;
        lv_label_set_text_fmt(_lblCurrentValue, "%.1f", absCurrent);
        
        lv_color_t curColor;
        const char* dirText;
        if (_current > 0.1f) {
            curColor = COL2_GREEN;
            dirText = LV_SYMBOL_UP " CHG";
        } else if (_current < -0.1f) {
            curColor = COL2_RED;
            dirText = LV_SYMBOL_DOWN " DSG";
        } else {
            curColor = COL2_GREY;
            dirText = "IDLE";
        }
        
        lv_obj_set_style_text_color(_lblCurrentValue, curColor, 0);
        
        if (_lblCurrentDir) {
            lv_label_set_text(_lblCurrentDir, dirText);
            lv_obj_set_style_text_color(_lblCurrentDir, curColor, 0);
        }
        
        /* Update panel border color to match current direction */
        if (_panelCurrent) {
            lv_obj_set_style_border_color(_panelCurrent, curColor, 0);
        }
    }
    
    /* Power */
    if (_lblPowerValue) {
        lv_label_set_text_fmt(_lblPowerValue, "%.1f", _power);
    }
}

lv_color_t BatteryScreenV2::getSOCColor(uint8_t soc) {
    if (soc <= 20) return COL2_RED;
    if (soc <= 50) return COL2_ORANGE;
    if (soc <= 80) return COL2_YELLOW;
    return COL2_GREEN;
}

/*===========================================================================*/
/*                         LIFECYCLE                                          */
/*===========================================================================*/

void BatteryScreenV2::onEnter() {
    ScreenBase::onEnter();
    _lastUpdate = 0;
}

void BatteryScreenV2::onExit() {
    ScreenBase::onExit();
}

void BatteryScreenV2::update() {
    uint32_t now = millis();
    if (now - _lastUpdate > 1000) {
        _lastUpdate = now;
        
        /* Demo: cycle SOC for visual testing */
        static int8_t dir = -1;
        _soc += dir;
        if (_soc <= 5) dir = 1;
        if (_soc >= 100) dir = -1;
        
        _power = _voltage * (_current < 0 ? -_current : _current);
        
        updateArcGauge();
        updateParameterValues();
    }
}

/*===========================================================================*/
/*                        DATA SETTERS                                        */
/*===========================================================================*/

void BatteryScreenV2::setSOC(uint8_t percent) {
    if (percent > 100) percent = 100;
    _soc = percent;
    if (_active) updateArcGauge();
}

void BatteryScreenV2::setVoltage(float volts) {
    _voltage = volts;
    if (_active) updateParameterValues();
}

void BatteryScreenV2::setCurrent(float amps) {
    _current = amps;
    if (_active) updateParameterValues();
}

void BatteryScreenV2::setPower(float watts) {
    _power = watts;
    if (_active) updateParameterValues();
}

void BatteryScreenV2::setStatus(const char* status, uint32_t color) {
    if (_lblStatus) {
        lv_label_set_text(_lblStatus, status);
    }
    if (_statusDot) {
        lv_color_t c = lv_color_hex(color);
        lv_obj_set_style_bg_color(_statusDot, c, 0);
        lv_obj_set_style_shadow_color(_statusDot, c, 0);
    }
}

void BatteryScreenV2::setTemperature(float tempC) {
    _temperature = tempC;
}

/*===========================================================================*/
/*                        EVENT HANDLERS                                      */
/*===========================================================================*/

void BatteryScreenV2::onSwitchV1Click(lv_event_t* e) {
    SCREEN_MGR.navigateTo(ScreenId::BATTERY_V1, ScreenTransition::SLIDE_RIGHT);
}

void BatteryScreenV2::onBrightnessChange(lv_event_t* e) {
    lv_obj_t* slider = lv_event_get_target(e);
    int32_t val = lv_slider_get_value(slider);
    /* Convert 10-100% to 0-255 PWM duty */
    uint8_t duty = (uint8_t)((uint16_t)val * 255 / 100);
    ledcWrite(1, duty);  /* BL_PIN = 1 */
}

} // namespace UI
