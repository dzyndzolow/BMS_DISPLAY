/**
 * @file battery_screen_v1.h
 * @brief Battery Monitor Screen V1 - Large vertical battery icon with parameters
 * 
 * Displays a large vertical battery gauge on the left side with voltage,
 * current, power parameters and status on the right side.
 * Inspired by JK BMS V19 display layout.
 * 
 * @version 1.0.0
 */

#ifndef UI_SCREENS_BATTERY_SCREEN_V1_H
#define UI_SCREENS_BATTERY_SCREEN_V1_H

#include "screen_base.h"

namespace UI {

class BatteryScreenV1 : public ScreenBase {
public:
    BatteryScreenV1();
    ~BatteryScreenV1() override = default;
    
    /*==== ScreenBase Implementation ====*/
    bool create() override;
    void onEnter() override;
    void onExit() override;
    void update() override;
    const char* getName() const override { return "BatteryV1"; }
    
    /*==== Data setters (call from data source) ====*/
    void setSOC(uint8_t percent);
    void setVoltage(float volts);
    void setCurrent(float amps);
    void setPower(float watts);
    void setStatus(const char* status, uint32_t color);
    void setTemperature(float tempC);
    
private:
    /* Battery icon elements */
    lv_obj_t* _batteryBody;       /* Battery outline container */
    lv_obj_t* _batteryCap;        /* Battery cap (top nub) */
    lv_obj_t* _batteryFill;       /* Colored fill bar */
    lv_obj_t* _lblSOC;            /* SOC percentage label inside battery */
    lv_obj_t* _lblSOCBelow;       /* SOC percentage label below battery */
    
    /* Parameter panels */
    lv_obj_t* _panelVoltage;
    lv_obj_t* _lblVoltageValue;
    lv_obj_t* _lblVoltageUnit;
    
    lv_obj_t* _panelCurrent;
    lv_obj_t* _lblCurrentValue;
    lv_obj_t* _lblCurrentUnit;
    lv_obj_t* _lblCurrentDir;     /* Charging/Discharging indicator */
    
    lv_obj_t* _panelPower;
    lv_obj_t* _lblPowerValue;
    lv_obj_t* _lblPowerUnit;
    
    /* Status area */
    lv_obj_t* _statusDot;         /* LED-style colored dot */
    lv_obj_t* _lblStatus;
    
    /* Navigation */
    lv_obj_t* _btnSwitchV2;       /* Button to switch to V2 */
    
    /* Data */
    uint8_t _soc;
    float _voltage;
    float _current;
    float _power;
    float _temperature;
    uint32_t _lastUpdate;
    
    /* Helpers */
    void createBatteryIcon(lv_obj_t* parent);
    void createParameterPanels(lv_obj_t* parent);
    void createStatusArea(lv_obj_t* parent);
    void updateBatteryFill();
    void updateParameterValues();
    lv_color_t getSOCColor(uint8_t soc);
    
    /* Event handlers */
    static void onSwitchV2Click(lv_event_t* e);
};

} // namespace UI

#endif /* UI_SCREENS_BATTERY_SCREEN_V1_H */
