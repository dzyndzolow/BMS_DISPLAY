# 🎯 WIZJA ARCHITEKTURY - Zintegrowany System

> **Projekt:** ESP32-S3 LVGL + Web Framework (JC4827W543_Integrated)  
> **Data aktualizacji:** 8 grudnia 2024  
> **Status:** Aktywna Integracja  
> **Wersja Poprzednia:** VISION.md (LVGL only)

---

## 💡 GŁÓWNA KONCEPCJA - ERWEITERT

### Motto projektu
**"Zero Technical Debt, Seamless Integration, Real-Time Everywhere"**

Tworzymy system embedded, który:
- Jest **czytelny** dla AI i człowieka
- Jest **modularny** - każdy komponent wymienialny
- Jest **testowalny** - każda warstwa niezależna
- Jest **skalowalny** - łatwo dodać WiFi, WebSocket, sensory
- Jest **maintainable** - kod nie gnije z czasem
- **JE TERAZ ONLINE** - Real-time Web API + WebSocket komunikacja

---

## 🏛️ FUNDAMENTALNE ZASADY (Rozszerzone)

### 1. Single Source of Truth (Dla całego systemu)

**Problem:** WiFi credentials, display pins, API endpoints w 5+ plikach.

**Rozwiązanie:** Centralizacja konfiguracji.

```
include/
├── defaults.h          ← Hardware piny, timeouty
├── wifi_config.h       ← WiFi SSID, PORT, WebSocket
└── lv_conf.h          ← LVGL konfiguracja
```

**Każda zmiana** - jedno miejsce!

---

### 2. Separation of Concerns (Z Web Integration)

**Warstwa:** 4+1 warstwa (now network)

```
┌─────────────────────────────────────────────────────┐
│ UI Layer (LVGL)                                     │
│ • ScreenManager, HomeScreen                        │
│ • Tylko logika prezentacji                         │
│ • Zero hardware, zero network                      │
└──────────────────┬──────────────────────────────────┘
                   │
          EventBus │ (Pub/Sub)
                   │
    ┌──────────────┼──────────────┐
    │              │              │
┌───▼──────┐ ┌────▼─────┐ ┌─────▼──────────┐
│ HAL      │ │ Services │ │ Network Layer  │
│ Layer    │ │ Layer    │ │ (NEW)          │
│          │ │          │ │                │
│ Display  │ │ EventBus │ │ WebIntegration │
│ Touch    │ │ Logger   │ │ HTTP Routes    │
│ Storage  │ │          │ │ WebSocket      │
└──────────┘ └──────────┘ └────────────────┘
     │            │              │
     └────────────┼──────────────┘
                  │
              Hardware
```

**Zasada:** 
- UI nigdy nie mówi do Sieci bezpośrednio
- Network Layer nie zna szczegółów UI
- Komunikacja przez EventBus

---

### 3. Interface-Based Design (+ Network Abstraction)

```cpp
/* Istniejące interfejsy HAL */
class IDisplay { ... };
class ITouch { ... };
class IStorage { ... };

/* NOWE - Network Abstraction */
class INetworkService {
public:
    virtual bool init(const char* ssid, const char* pwd) = 0;
    virtual void publishEvent(const char* channel, JsonDocument& doc) = 0;
    virtual bool isConnected() const = 0;
};

/* Implementacja WebFramework */
class WebFrameworkService : public INetworkService {
    bool init(...) override { ... }
    void publishEvent(...) override { ... }
};
```

**Korzyść:** Możesz zmienić transport z HTTP na MQTT, bez zmiany UI!

---

### 4. Event-Driven Architecture (Extended)

**Strumienie zdarzeń:**

```
UI Events          →  ┌─────────┐     →  Network Events
┌─────────────────┐   │ EventBus│        ┌──────────────┐
│ Button Click    │   │         │        │ Brightness   │
│ Screen Change   │──→│ Publish │───┬───→│ Changed      │
│ Settings Update │   │ Subscribe    │    │ Screen Info  │
└─────────────────┘   └─────────┘   │    │ Sensor Data  │
                                    │    └──────────────┘
Hardware Events     ←  ┌─────────┐  │
┌─────────────────┐   │ EventBus│  │
│ Temperature     │   │         │  │
│ Touch Input     │←──│ Subscribe──┘
│ SD Card Status  │   │
└─────────────────┘   └─────────┘
```

---

## 🌐 NOWE: NETWORK ARCHITECTURE

### WebFramework Integration Layer

```
┌─────────────────────────────────────────────┐
│ lib/espwebframework/                        │
│ • Django-like Router (/api/*, /ws/*)       │
│ • Template Engine (for web UI)              │
│ • WebSocket Channels & Rooms                │
│ • ORM + JSON Persistence                    │
│ • Scheduler (Cron-like tasks)               │
└─────────────────────────────────────────────┘
              ↓
┌─────────────────────────────────────────────┐
│ src/integration/web_integration.*           │
│ • WebIntegration Singleton                  │
│ • WiFi Management (STA mode)                │
│ • Route Definitions                         │
│ • WebSocket Event Publishing                │
└─────────────────────────────────────────────┘
              ↓
┌─────────────────────────────────────────────┐
│ include/wifi_config.h                       │
│ • SSID, PASSWORD (SSoT)                     │
│ • Port Configuration                        │
│ • Update Intervals                          │
│ • Enable/Disable Flag                       │
└─────────────────────────────────────────────┘
```

### REST API Struktura

```
GET /                          → Status page (JSON)
GET /api/status                → System metrics
POST /api/brightness           → Set display brightness
GET /api/screen                → Current screen info
GET /api/config                → Device configuration
POST /api/config               → Update configuration
GET /api/sensors               → Sensor readings
DELETE /api/cache              → Clear cache

WebSocket:
/ws/ui-events                  → UI state changes
/ws/sensors                    → Real-time sensor data
/ws/commands                   → Commands from clients
```

---

## 📊 METRYKI SYSTEMU (Po Integracji)

| Aspekt | Wartoś | Limit | Status |
|--------|--------|-------|--------|
| **Flash (Code)** | ~1.2MB | 4.0MB | ✅ OK |
| **Flash (SPIFFS)** | ~448KB | 448KB | ✅ Full |
| **RAM** | ~50KB | 327KB | ✅ OK |
| **PSRAM** | <10MB | 8MB | ⚠️ Watch |
| **Files** | ~50+ | - | ✅ Organized |
| **Tasks** | 4 | - | ✅ Balanced |

---

## 🔄 Workflow: Dodanie Nowej Funkcji

### Przykład: "Toggle Night Mode"

#### Krok 1: UI Implementation
```cpp
// ui/screens/settings_screen.cpp
static void night_mode_callback(lv_event_t * e) {
    bool enabled = lv_obj_get_state(btn_night_mode) & LV_STATE_CHECKED;
    
    // Update display
    DisplayManager::getInstance().setNightMode(enabled);
    
    // Publish event
    EventBus::getInstance().publish("UI", "night_mode_toggled", enabled);
}
```

#### Krok 2: EventBus Listener
```cpp
// services/ui_service.cpp
EventBus::getInstance().subscribe("UI", "night_mode_toggled", 
    [](bool enabled) {
        Logger::log("UI", "Night mode: " + String(enabled));
    }
);
```

#### Krok 3: Network Publishing
```cpp
// integration/web_integration.cpp
EventBus::getInstance().subscribe("UI", "night_mode_toggled",
    [](bool enabled) {
        JsonDocument doc;
        doc["event"] = "night_mode_changed";
        doc["enabled"] = enabled;
        
        WebIntegration::getInstance().broadcastMessage(
            "/ws/ui-events", 
            doc.as<String>()
        );
    }
);
```

#### Krok 4: Web API
```cpp
// Add new endpoint
Routes().get("/api/night-mode", [](Request& req) {
    JsonDocument doc;
    doc["enabled"] = DisplayManager::getInstance().isNightMode();
    return Response::json(doc);
});

Routes().post("/api/night-mode", [](Request& req) {
    JsonDocument reqDoc;
    deserializeJson(reqDoc, req.body);
    
    bool enabled = reqDoc["enabled"].as<bool>();
    DisplayManager::getInstance().setNightMode(enabled);
    
    JsonDocument resDoc;
    resDoc["status"] = "updated";
    return Response::json(resDoc);
});
```

---

## 🎨 Nowe Możliwości

### Real-Time Monitoring
```javascript
// Browser
const ws = new WebSocket('ws://device.local:81/ws/sensors');
ws.onmessage = (e) => {
    let data = JSON.parse(e.data);
    updateChart(data.sensor, data.value);
};
```

### Remote Configuration
```bash
# Change settings via API
curl -X POST http://device.local/api/config \
  -H "Content-Type: application/json" \
  -d '{"brightness": 50, "nightMode": true}'
```

### Logging & Analytics
- Wszystkie zdarzenia UI logowane
- WebSocket events historyzowane
- ORM persistence na SPIFFS

---

## 📋 Migration Checklist

- [x] Kopia JC4827W543_PIO_develop do JC4827W543_Integrated
- [x] Dodaj espwebframework z arduino_web
- [x] web_integration.h/cpp - singleton
- [x] wifi_config.h - SSoT
- [x] main.cpp - nowy entry point
- [ ] Przetestuj LVGL display
- [ ] Przetestuj WiFi connection
- [ ] Testy HTTP endpoints
- [ ] Testy WebSocket channels
- [ ] Memory profiling
- [ ] OTA Update support

---

## 🚀 Następne Kroki

1. **Phase 1 (Now):** Basic WiFi + REST API
2. **Phase 2:** WebSocket real-time updates
3. **Phase 3:** OTA Updates over WiFi
4. **Phase 4:** Advanced analytics + logging
5. **Phase 5:** Mobile app companion

