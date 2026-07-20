/**
 * @file screen_manager.cpp
 * @brief Screen Manager implementation
 * 
 * @version 1.0.0 (MASTER_PLAN Phase 4)
 */

#include "screen_manager.h"
#include "../../services/event_bus.h"
#include <Arduino.h>

namespace UI {

ScreenManager& ScreenManager::instance() {
    static ScreenManager instance;
    return instance;
}

ScreenManager::ScreenManager()
    : _screenCount(0)
    , _currentScreenId(ScreenId::SPLASH)
    , _historyIndex(0)
{
    for (uint8_t i = 0; i < MAX_SCREENS; i++) {
        _screens[i] = nullptr;
    }
    for (uint8_t i = 0; i < HISTORY_DEPTH; i++) {
        _history[i] = ScreenId::SPLASH;
    }
}

bool ScreenManager::registerScreen(ScreenBase* screen) {
    if (!screen) return false;
    
    uint8_t idx = static_cast<uint8_t>(screen->getId());
    if (idx >= MAX_SCREENS) {
        Serial.println("[ScreenMgr] ERROR: Screen ID out of range!");
        return false;
    }
    
    if (_screens[idx] != nullptr) {
        Serial.printf("[ScreenMgr] WARNING: Replacing screen %s\n", 
            _screens[idx]->getName());
        delete _screens[idx];
    }
    
    _screens[idx] = screen;
    _screenCount++;
    
    Serial.printf("[ScreenMgr] Registered: %s (id=%d)\n", screen->getName(), idx);
    return true;
}

ScreenBase* ScreenManager::getScreen(ScreenId id) {
    uint8_t idx = static_cast<uint8_t>(id);
    if (idx >= MAX_SCREENS) return nullptr;
    return _screens[idx];
}

bool ScreenManager::navigateTo(ScreenId id, ScreenTransition transition, bool pushHistory) {
    ScreenBase* target = getScreen(id);
    
    if (!target) {
        Serial.printf("[ScreenMgr] ERROR: Screen %d not registered!\n", static_cast<int>(id));
        return false;
    }
    
    /* Create screen if needed */
    if (!target->isCreated()) {
        if (!target->create()) {
            Serial.printf("[ScreenMgr] ERROR: Failed to create screen %s\n", target->getName());
            return false;
        }
    }
    
    /* Get current screen for exit callback */
    ScreenBase* current = getCurrentScreen();
    
    /* Push current to history if requested */
    if (pushHistory && current && _historyIndex < HISTORY_DEPTH) {
        _history[_historyIndex++] = _currentScreenId;
    }
    
    /* Exit current screen */
    if (current) {
        current->onExit();
    }
    
    /* Apply transition and load new screen */
    applyTransition(target->getScreen(), transition);
    lv_scr_load(target->getScreen());
    
    /* Update state */
    _currentScreenId = id;
    
    /* Enter new screen */
    target->onEnter();
    
    /* Publish screen change event */
    Services::ScreenEvent event;
    event.type = Services::EventType::UI_SCREEN_CHANGE;
    event.timestamp = millis();
    event.sourceId = 0;
    event.fromScreen = current ? static_cast<uint8_t>(current->getId()) : 0;
    event.toScreen = static_cast<uint8_t>(id);
    Services::EventBus::instance().publish(event);
    
    Serial.printf("[ScreenMgr] Navigated to: %s\n", target->getName());
    return true;
}

bool ScreenManager::goBack(ScreenTransition transition) {
    if (!canGoBack()) {
        return false;
    }
    
    _historyIndex--;
    ScreenId previousId = _history[_historyIndex];
    
    return navigateTo(previousId, transition, false);
}

bool ScreenManager::canGoBack() const {
    return _historyIndex > 0;
}

void ScreenManager::clearHistory() {
    _historyIndex = 0;
}

ScreenBase* ScreenManager::getCurrentScreen() {
    return getScreen(_currentScreenId);
}

ScreenId ScreenManager::getCurrentScreenId() const {
    return _currentScreenId;
}

void ScreenManager::update() {
    ScreenBase* current = getCurrentScreen();
    if (current && current->isActive()) {
        current->update();
    }
}

void ScreenManager::applyTransition(lv_obj_t* screen, ScreenTransition transition) {
    if (!screen) return;
    
    switch (transition) {
        case ScreenTransition::FADE:
            lv_scr_load_anim(screen, LV_SCR_LOAD_ANIM_FADE_ON, 300, 0, false);
            break;
        case ScreenTransition::SLIDE_LEFT:
            lv_scr_load_anim(screen, LV_SCR_LOAD_ANIM_MOVE_LEFT, 300, 0, false);
            break;
        case ScreenTransition::SLIDE_RIGHT:
            lv_scr_load_anim(screen, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 300, 0, false);
            break;
        case ScreenTransition::SLIDE_UP:
            lv_scr_load_anim(screen, LV_SCR_LOAD_ANIM_MOVE_TOP, 300, 0, false);
            break;
        case ScreenTransition::SLIDE_DOWN:
            lv_scr_load_anim(screen, LV_SCR_LOAD_ANIM_MOVE_BOTTOM, 300, 0, false);
            break;
        case ScreenTransition::NONE:
        default:
            /* No animation, just load */
            break;
    }
}

} // namespace UI
