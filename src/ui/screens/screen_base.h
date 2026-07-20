/**
 * @file screen_base.h
 * @brief Base class for all UI screens
 * 
 * Provides common interface and lifecycle management for screens.
 * 
 * @version 1.0.0 (MASTER_PLAN Phase 4)
 */

#ifndef UI_SCREEN_BASE_H
#define UI_SCREEN_BASE_H

#include <lvgl.h>
#include <cstdint>

namespace UI {

/**
 * @brief Screen identifiers
 */
enum class ScreenId : uint8_t {
    SPLASH      = 0,
    HOME        = 1,
    SETTINGS    = 2,
    DEMO        = 3,
    BATTERY_V1  = 4,
    BATTERY_V2  = 5,
    
    /* Add new screens here */
    
    SCREEN_COUNT
};

/**
 * @brief Screen transition types
 */
enum class ScreenTransition : uint8_t {
    NONE,
    FADE,
    SLIDE_LEFT,
    SLIDE_RIGHT,
    SLIDE_UP,
    SLIDE_DOWN
};

/**
 * @brief Base class for all screens
 */
class ScreenBase {
public:
    /**
     * @brief Constructor
     * @param id Screen identifier
     */
    explicit ScreenBase(ScreenId id);
    
    /**
     * @brief Virtual destructor
     */
    virtual ~ScreenBase();
    
    /*==== Lifecycle ====*/
    
    /**
     * @brief Create screen content (called once)
     * @return true on success
     */
    virtual bool create() = 0;
    
    /**
     * @brief Called when screen becomes active
     */
    virtual void onEnter();
    
    /**
     * @brief Called when screen becomes inactive
     */
    virtual void onExit();
    
    /**
     * @brief Update screen content (called periodically if active)
     */
    virtual void update();
    
    /**
     * @brief Destroy screen content (free resources)
     */
    virtual void destroy();
    
    /*==== Properties ====*/
    
    /**
     * @brief Get screen ID
     */
    ScreenId getId() const { return _id; }
    
    /**
     * @brief Get LVGL screen object
     */
    lv_obj_t* getScreen() const { return _screen; }
    
    /**
     * @brief Check if screen is created
     */
    bool isCreated() const { return _screen != nullptr; }
    
    /**
     * @brief Check if screen is currently active
     */
    bool isActive() const { return _active; }
    
    /**
     * @brief Get screen name (for debugging)
     */
    virtual const char* getName() const = 0;
    
protected:
    ScreenId _id;
    lv_obj_t* _screen;
    bool _active;
    
    /**
     * @brief Helper to create screen container
     */
    lv_obj_t* createScreenContainer();
    
    /**
     * @brief Helper to add title bar
     */
    lv_obj_t* addTitleBar(const char* title);
    
    /**
     * @brief Helper to add back button
     */
    lv_obj_t* addBackButton();
};

} // namespace UI

#endif /* UI_SCREEN_BASE_H */
