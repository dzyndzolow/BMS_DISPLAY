/**
 * @file screen_manager.h
 * @brief Screen Manager - handles screen navigation and transitions
 * 
 * @version 1.0.0 (MASTER_PLAN Phase 4)
 */

#ifndef UI_SCREEN_MANAGER_H
#define UI_SCREEN_MANAGER_H

#include "screen_base.h"
#include <cstdint>

namespace UI {

/**
 * @brief Maximum number of screens that can be registered
 */
constexpr uint8_t MAX_SCREENS = 16;

/**
 * @brief Navigation history depth
 */
constexpr uint8_t HISTORY_DEPTH = 8;

/**
 * @brief Screen Manager singleton
 */
class ScreenManager {
public:
    /**
     * @brief Get singleton instance
     */
    static ScreenManager& instance();
    
    /*==== Registration ====*/
    
    /**
     * @brief Register a screen with the manager
     * @param screen Pointer to screen (manager takes ownership)
     * @return true on success
     */
    bool registerScreen(ScreenBase* screen);
    
    /**
     * @brief Get registered screen by ID
     */
    ScreenBase* getScreen(ScreenId id);
    
    /*==== Navigation ====*/
    
    /**
     * @brief Navigate to a screen
     * @param id Target screen ID
     * @param transition Transition animation
     * @param pushHistory Add current screen to back history
     * @return true on success
     */
    bool navigateTo(ScreenId id, ScreenTransition transition = ScreenTransition::NONE, bool pushHistory = true);
    
    /**
     * @brief Go back to previous screen
     * @param transition Transition animation
     * @return true if there was a screen to go back to
     */
    bool goBack(ScreenTransition transition = ScreenTransition::SLIDE_RIGHT);
    
    /**
     * @brief Check if back navigation is possible
     */
    bool canGoBack() const;
    
    /**
     * @brief Clear navigation history
     */
    void clearHistory();
    
    /*==== Current State ====*/
    
    /**
     * @brief Get current active screen
     */
    ScreenBase* getCurrentScreen();
    
    /**
     * @brief Get current screen ID
     */
    ScreenId getCurrentScreenId() const;
    
    /**
     * @brief Update current screen (call in main loop)
     */
    void update();
    
private:
    ScreenManager();
    ~ScreenManager() = default;
    
    ScreenManager(const ScreenManager&) = delete;
    ScreenManager& operator=(const ScreenManager&) = delete;
    
    ScreenBase* _screens[MAX_SCREENS];
    uint8_t _screenCount;
    
    ScreenId _currentScreenId;
    ScreenId _history[HISTORY_DEPTH];
    uint8_t _historyIndex;
    
    void applyTransition(lv_obj_t* screen, ScreenTransition transition);
};

/**
 * @brief Convenience macro for screen manager access
 */
#define SCREEN_MGR UI::ScreenManager::instance()

} // namespace UI

#endif /* UI_SCREEN_MANAGER_H */
