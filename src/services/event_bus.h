/**
 * @file event_bus.h
 * @brief Type-safe Event Bus for decoupled module communication
 * 
 * Enables loose coupling between system components through
 * publish/subscribe pattern with compile-time type safety.
 * 
 * @version 1.0.0 (MASTER_PLAN Phase 3)
 */

#ifndef SERVICES_EVENT_BUS_H
#define SERVICES_EVENT_BUS_H

#include <cstdint>
#include <functional>

namespace Services {

/*===========================================================================*/
/*                              EVENT TYPES                                   */
/*===========================================================================*/

/**
 * @brief System event identifiers
 */
enum class EventType : uint16_t {
    /* System events */
    SYSTEM_BOOT_COMPLETE    = 0x0001,
    SYSTEM_LOW_MEMORY       = 0x0002,
    SYSTEM_ERROR            = 0x0003,
    SYSTEM_SHUTDOWN         = 0x0004,
    
    /* Display events */
    DISPLAY_READY           = 0x0100,
    DISPLAY_BRIGHTNESS      = 0x0101,
    DISPLAY_ROTATION        = 0x0102,
    
    /* Touch events */
    TOUCH_PRESSED           = 0x0200,
    TOUCH_RELEASED          = 0x0201,
    TOUCH_GESTURE           = 0x0202,
    
    /* Storage events */
    STORAGE_MOUNTED         = 0x0300,
    STORAGE_UNMOUNTED       = 0x0301,
    STORAGE_ERROR           = 0x0302,
    STORAGE_LOW_SPACE       = 0x0303,
    
    /* UI events */
    UI_SCREEN_CHANGE        = 0x0400,
    UI_BUTTON_CLICK         = 0x0401,
    UI_SLIDER_CHANGE        = 0x0402,
    UI_CHECKBOX_TOGGLE      = 0x0403,
    
    /* Network events (future) */
    WIFI_CONNECTED          = 0x0500,
    WIFI_DISCONNECTED       = 0x0501,
    OTA_AVAILABLE           = 0x0502,
    OTA_PROGRESS            = 0x0503,
    
    /* Application events */
    APP_CUSTOM_START        = 0x1000,
    /* Add custom events starting from 0x1000 */
};

/*===========================================================================*/
/*                             EVENT DATA                                     */
/*===========================================================================*/

/**
 * @brief Base event data structure
 */
struct EventData {
    EventType type;
    uint32_t timestamp;     /* millis() when event was created */
    uint16_t sourceId;      /* ID of the module that created the event */
};

/**
 * @brief Integer value event
 */
struct IntEvent : EventData {
    int32_t value;
};

/**
 * @brief Float value event
 */
struct FloatEvent : EventData {
    float value;
};

/**
 * @brief Touch event data
 */
struct TouchEvent : EventData {
    int16_t x;
    int16_t y;
    uint8_t touchId;
};

/**
 * @brief Screen change event
 */
struct ScreenEvent : EventData {
    uint8_t fromScreen;
    uint8_t toScreen;
};

/**
 * @brief Error event
 */
struct ErrorEvent : EventData {
    uint16_t errorCode;
    char message[64];
};

/**
 * @brief Generic event with raw data pointer
 */
struct RawEvent : EventData {
    void* data;
    size_t dataSize;
};

/*===========================================================================*/
/*                            CALLBACK TYPES                                  */
/*===========================================================================*/

/**
 * @brief Generic event callback
 */
using EventCallback = std::function<void(const EventData&)>;

/**
 * @brief Typed event callbacks
 */
using IntEventCallback = std::function<void(const IntEvent&)>;
using FloatEventCallback = std::function<void(const FloatEvent&)>;
using TouchEventCallback = std::function<void(const TouchEvent&)>;
using ScreenEventCallback = std::function<void(const ScreenEvent&)>;
using ErrorEventCallback = std::function<void(const ErrorEvent&)>;

/**
 * @brief Subscription handle for unsubscribing
 */
using SubscriptionId = uint32_t;

/*===========================================================================*/
/*                            EVENT BUS CLASS                                 */
/*===========================================================================*/

/**
 * @brief Maximum number of subscribers per event type
 */
constexpr uint8_t MAX_SUBSCRIBERS_PER_EVENT = 8;

/**
 * @brief Maximum number of event types with subscribers
 */
constexpr uint8_t MAX_EVENT_TYPES = 32;

/**
 * @brief Event Bus singleton - manages all event subscriptions and publishing
 */
class EventBus {
public:
    /**
     * @brief Get singleton instance
     */
    static EventBus& instance();
    
    /*==== Subscription ====*/
    
    /**
     * @brief Subscribe to an event type
     * @param type Event type to subscribe to
     * @param callback Callback function
     * @return Subscription ID for unsubscribing, 0 on failure
     */
    SubscriptionId subscribe(EventType type, EventCallback callback);
    
    /**
     * @brief Unsubscribe using subscription ID
     * @param id Subscription ID returned from subscribe()
     * @return true if successfully unsubscribed
     */
    bool unsubscribe(SubscriptionId id);
    
    /**
     * @brief Unsubscribe all callbacks for an event type
     * @param type Event type
     */
    void unsubscribeAll(EventType type);
    
    /*==== Publishing ====*/
    
    /**
     * @brief Publish an event (synchronous - callbacks called immediately)
     * @param event Event data to publish
     */
    void publish(const EventData& event);
    
    /**
     * @brief Publish an event by type only (no additional data)
     * @param type Event type
     * @param sourceId Source module ID
     */
    void publish(EventType type, uint16_t sourceId = 0);
    
    /**
     * @brief Publish integer event
     */
    void publishInt(EventType type, int32_t value, uint16_t sourceId = 0);
    
    /**
     * @brief Publish float event
     */
    void publishFloat(EventType type, float value, uint16_t sourceId = 0);
    
    /**
     * @brief Publish touch event
     */
    void publishTouch(EventType type, int16_t x, int16_t y, uint8_t touchId = 0, uint16_t sourceId = 0);
    
    /**
     * @brief Publish error event
     */
    void publishError(uint16_t errorCode, const char* message, uint16_t sourceId = 0);
    
    /*==== Statistics ====*/
    
    /**
     * @brief Get number of subscribers for an event type
     */
    uint8_t getSubscriberCount(EventType type) const;
    
    /**
     * @brief Get total number of events published since boot
     */
    uint32_t getTotalEventsPublished() const;
    
    /**
     * @brief Reset statistics
     */
    void resetStats();
    
private:
    EventBus();
    ~EventBus() = default;
    
    /* Prevent copying */
    EventBus(const EventBus&) = delete;
    EventBus& operator=(const EventBus&) = delete;
    
    /* Internal structures */
    struct Subscription {
        EventType type;
        EventCallback callback;
        SubscriptionId id;
        bool active;
    };
    
    Subscription _subscriptions[MAX_EVENT_TYPES * MAX_SUBSCRIBERS_PER_EVENT];
    uint16_t _subscriptionCount;
    SubscriptionId _nextId;
    uint32_t _totalEventsPublished;
    
    /* Find subscription by ID */
    Subscription* findSubscription(SubscriptionId id);
};

/*===========================================================================*/
/*                           HELPER MACROS                                    */
/*===========================================================================*/

/**
 * @brief Convenience macro for getting EventBus instance
 */
#define EVENT_BUS Services::EventBus::instance()

/**
 * @brief Convenience macro for publishing simple events
 */
#define PUBLISH_EVENT(type) EVENT_BUS.publish(Services::EventType::type)

/**
 * @brief Convenience macro for subscribing
 */
#define SUBSCRIBE_EVENT(type, callback) \
    EVENT_BUS.subscribe(Services::EventType::type, callback)

} // namespace Services

#endif /* SERVICES_EVENT_BUS_H */
