/**
 * @file home_screen.h
 * @brief Home Screen - main application screen
 * 
 * Example screen implementation showing how to use ScreenBase.
 * 
 * @version 1.0.0 (MASTER_PLAN Phase 4)
 */

#ifndef UI_SCREENS_HOME_SCREEN_H
#define UI_SCREENS_HOME_SCREEN_H

#include "screen_base.h"

namespace UI {

class HomeScreen : public ScreenBase {
public:
    HomeScreen();
    ~HomeScreen() override = default;
    
    /*==== ScreenBase Implementation ====*/
    bool create() override;
    void onEnter() override;
    void onExit() override;
    void update() override;
    const char* getName() const override { return "Home"; }
    
private:
    /* UI Elements */
    lv_obj_t* _lblTitle;
    lv_obj_t* _lblStatus;
    lv_obj_t* _btnDemo;
    lv_obj_t* _btnSettings;
    lv_obj_t* _lblMemory;
    
    uint32_t _lastUpdate;
    
    /* Event handlers */
    static void onDemoClick(lv_event_t* e);
    static void onSettingsClick(lv_event_t* e);
    
    void updateMemoryLabel();
};

} // namespace UI

#endif /* UI_SCREENS_HOME_SCREEN_H */
