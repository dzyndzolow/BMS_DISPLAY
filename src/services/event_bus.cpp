/**
 * @file event_bus.cpp
 * @brief Event Bus Implementation
 * 
 * @version 1.0.0 (MASTER_PLAN Phase 3)
 */

#include "event_bus.h"
#include <Arduino.h>
#include <cstring>

namespace Services {

/*===========================================================================*/
/*                          SINGLETON INSTANCE                                */
/*===========================================================================*/

EventBus& EventBus::instance() {
    static EventBus instance;
    return instance;
}

/*===========================================================================*/
/*                            CONSTRUCTOR                                     */
/*===========================================================================*/

EventBus::EventBus()
    : _subscriptionCount(0)
    , _nextId(1)
    , _totalEventsPublished(0)
{
    /* Initialize all subscriptions as inactive */
    for (uint16_t i = 0; i < MAX_EVENT_TYPES * MAX_SUBSCRIBERS_PER_EVENT; i++) {
        _subscriptions[i].active = false;
        _subscriptions[i].id = 0;
    }
}

/*===========================================================================*/
/*                            SUBSCRIPTION                                    */
/*===========================================================================*/

SubscriptionId EventBus::subscribe(EventType type, EventCallback callback) {
    /* Find empty slot */
    for (uint16_t i = 0; i < MAX_EVENT_TYPES * MAX_SUBSCRIBERS_PER_EVENT; i++) {
        if (!_subscriptions[i].active) {
            _subscriptions[i].type = type;
            _subscriptions[i].callback = callback;
            _subscriptions[i].id = _nextId++;
            _subscriptions[i].active = true;
            _subscriptionCount++;
            
            Serial.printf("[EventBus] Subscribed to event 0x%04X (id: %lu)\n",
                (uint16_t)type, (unsigned long)_subscriptions[i].id);
            
            return _subscriptions[i].id;
        }
    }
    
    Serial.println("[EventBus] ERROR: No subscription slots available!");
    return 0;
}

bool EventBus::unsubscribe(SubscriptionId id) {
    Subscription* sub = findSubscription(id);
    if (sub) {
        sub->active = false;
        sub->callback = nullptr;
        _subscriptionCount--;
        
        Serial.printf("[EventBus] Unsubscribed id: %lu\n", (unsigned long)id);
        return true;
    }
    return false;
}

void EventBus::unsubscribeAll(EventType type) {
    for (uint16_t i = 0; i < MAX_EVENT_TYPES * MAX_SUBSCRIBERS_PER_EVENT; i++) {
        if (_subscriptions[i].active && _subscriptions[i].type == type) {
            _subscriptions[i].active = false;
            _subscriptions[i].callback = nullptr;
            _subscriptionCount--;
        }
    }
    Serial.printf("[EventBus] Unsubscribed all from event 0x%04X\n", (uint16_t)type);
}

EventBus::Subscription* EventBus::findSubscription(SubscriptionId id) {
    for (uint16_t i = 0; i < MAX_EVENT_TYPES * MAX_SUBSCRIBERS_PER_EVENT; i++) {
        if (_subscriptions[i].active && _subscriptions[i].id == id) {
            return &_subscriptions[i];
        }
    }
    return nullptr;
}

/*===========================================================================*/
/*                             PUBLISHING                                     */
/*===========================================================================*/

void EventBus::publish(const EventData& event) {
    _totalEventsPublished++;
    
    /* Find and call all matching subscribers */
    for (uint16_t i = 0; i < MAX_EVENT_TYPES * MAX_SUBSCRIBERS_PER_EVENT; i++) {
        if (_subscriptions[i].active && _subscriptions[i].type == event.type) {
            if (_subscriptions[i].callback) {
                _subscriptions[i].callback(event);
            }
        }
    }
}

void EventBus::publish(EventType type, uint16_t sourceId) {
    EventData event;
    event.type = type;
    event.timestamp = millis();
    event.sourceId = sourceId;
    publish(event);
}

void EventBus::publishInt(EventType type, int32_t value, uint16_t sourceId) {
    IntEvent event;
    event.type = type;
    event.timestamp = millis();
    event.sourceId = sourceId;
    event.value = value;
    publish(event);
}

void EventBus::publishFloat(EventType type, float value, uint16_t sourceId) {
    FloatEvent event;
    event.type = type;
    event.timestamp = millis();
    event.sourceId = sourceId;
    event.value = value;
    publish(event);
}

void EventBus::publishTouch(EventType type, int16_t x, int16_t y, uint8_t touchId, uint16_t sourceId) {
    TouchEvent event;
    event.type = type;
    event.timestamp = millis();
    event.sourceId = sourceId;
    event.x = x;
    event.y = y;
    event.touchId = touchId;
    publish(event);
}

void EventBus::publishError(uint16_t errorCode, const char* message, uint16_t sourceId) {
    ErrorEvent event;
    event.type = EventType::SYSTEM_ERROR;
    event.timestamp = millis();
    event.sourceId = sourceId;
    event.errorCode = errorCode;
    strncpy(event.message, message, sizeof(event.message) - 1);
    event.message[sizeof(event.message) - 1] = '\0';
    publish(event);
}

/*===========================================================================*/
/*                             STATISTICS                                     */
/*===========================================================================*/

uint8_t EventBus::getSubscriberCount(EventType type) const {
    uint8_t count = 0;
    for (uint16_t i = 0; i < MAX_EVENT_TYPES * MAX_SUBSCRIBERS_PER_EVENT; i++) {
        if (_subscriptions[i].active && _subscriptions[i].type == type) {
            count++;
        }
    }
    return count;
}

uint32_t EventBus::getTotalEventsPublished() const {
    return _totalEventsPublished;
}

void EventBus::resetStats() {
    _totalEventsPublished = 0;
}

} // namespace Services
