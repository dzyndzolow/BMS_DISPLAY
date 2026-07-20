/**
 * @file battery_screen_v2.h
 * @brief Battery Monitor Screen V2 - Circular arc gauge with parameters
 * 
 * Displays a large circular arc SOC indicator on the left side with voltage,
 * current, power parameters and status on the right side.
 * 
 * @version 1.0.0
 */

#ifndef UI_SCREENS_BATTERY_SCREEN_V2_H
#define UI_SCREENS_BATTERY_SCREEN_V2_H

#include "screen_base.h"

namespace UI {

class BatteryScreenV2 : public ScreenBase {
public:
    BatteryScreenV2();
    ~BatteryScreenV2() override = default;
    
    /*==== ScreenBase Implementation ====*/
    bool create() override;
    void onEnter() override;
    void onExit() override;
    void update() override;
    const char* getName() const override { return "BatteryV2"; }
    
    /*==== Data setters (call from data source) ====*/
    void setSOC(uint8_t percent);
    void setVoltage(float volts);
    void setCurrent(float amps);
    void setPower(float watts);
    void setStatus(const char* status, uint32_t color);
    void setTemperature(float tempC);
    
private:
    /* Arc gauge elements */
    lv_obj_t* _arcGauge;          /* Main arc indicator */
    lv_obj_t* _lblSOCValue;       /* Large SOC percentage in center */
    lv_obj_t* _lblSOCUnit;        /* "%" label */
    lv_obj_t* _lblSOCLabel;       /* "SOC" label below percentage */
    
    /* Parameter panels */
    lv_obj_t* _panelVoltage;
    lv_obj_t* _lblVoltageIcon;
    lv_obj_t* _lblVoltageName;
    lv_obj_t* _lblVoltageValue;
    
    lv_obj_t* _panelCurrent;
    lv_obj_t* _lblCurrentIcon;
    lv_obj_t* _lblCurrentName;
    lv_obj_t* _lblCurrentValue;
    lv_obj_t* _lblCurrentDir;
    
    lv_obj_t* _panelPower;
    lv_obj_t* _lblPowerIcon;
    lv_obj_t* _lblPowerName;
    lv_obj_t* _lblPowerValue;
    
    /* Status area */
    lv_obj_t* _statusDot;
    lv_obj_t* _lblStatus;
    
    /* Brightness control */
    lv_obj_t* _sliderBrightness;
    lv_obj_t* _lblBrightnessIcon;
    
    /* Navigation */
    lv_obj_t* _btnSwitchV1;
    
    /* Data */
    uint8_t _soc;
    float _voltage;
    float _current;
    float _power;
    float _temperature;
    uint32_t _lastUpdate;
    
    /* Helpers */
    void createArcGauge(lv_obj_t* parent);
    void createParameterPanels(lv_obj_t* parent);
    void createStatusArea(lv_obj_t* parent);
    void updateArcGauge();
    void updateParameterValues();
    lv_color_t getSOCColor(uint8_t soc);
    
    /* Event handlers */
    static void onSwitchV1Click(lv_event_t* e);
    static void onBrightnessChange(lv_event_t* e);
};

} // namespace UI

#endif /* UI_SCREENS_BATTERY_SCREEN_V2_H */
