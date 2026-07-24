# 📚 DEVELOPMENT GUIDE - Web Integration

> **Wersja:** 1.0.0 Integrated  
> **Data:** 8 grudnia 2024  
> **Zakres:** Rozbudowa systemu z Web Framework

---

## 📖 INTRODUCTION

Ten dokument rozszerza oryginalny DEVELOPMENT_GUIDE.md z sekcji dotyczącej **integracji Web Framework** z istniejącą aplikacją LVGL.

**Przeczytaj najpierw:** `docs/DEVELOPMENT_GUIDE.md` (podstawowe koncepty)

---

## 🌐 SEKCJA VII: WEB FRAMEWORK INTEGRATION

### 7.1 Architektura Integracji

Web Framework integruje się z istniejącą aplikacją poprzez:

1. **Separation of Concerns** - oddzielne warstwy
2. **EventBus** - asynchroniczna komunikacja
3. **Singleton Pattern** - WebIntegration zarządzający siecią
4. **Multi-tasking** - LVGL i Web Server na osobnych rdzeniach

```
Warstwa UI          Warstwa Sieciowa
┌──────────────┐   ┌──────────────────┐
│ LVGL Display │   │ HTTP Routes      │
│ Touch Input  │   │ WebSocket        │
│ ScreenMgr    │   │ JSON API         │
└──────┬───────┘   └────────┬─────────┘
       │                    │
       └────────EventBus────┘
```

### 7.2 Konfiguracja Początkowa

#### Krok 1: Ustaw WiFi Credentials

**Plik:** `include/wifi_config.h`

```cpp
namespace WifiConfig {
    constexpr const char* SSID = "YourNetwork";
    constexpr const char* PASSWORD = "YourPassword";
    constexpr bool ENABLE_WIFI_ON_BOOT = true;
}
```

#### Krok 2: Weryfikuj platformio.ini

Sprawdź, czy zawiera obie biblioteki:

```ini
lib_deps = 
    moononournation/GFX Library for Arduino@^1.4.7
    lvgl/lvgl@^8.3.11
    bblanchon/ArduinoJson@^7.0.0
    gilmaimon/ArduinoWebsockets@^0.5.3
```

#### Krok 3: Kompilacja

```bash
platformio run -e esp32-s3-devkitc1-n4r8-usb
```

---

## 🛠️ SEKCJA VIII: DODAWANIE NETWORK ENDPOINTS

### 8.1 Struktura Nowego Endpoint

#### Definicja w web_integration.cpp

```cpp
void WebIntegration::start() {
    // GET /api/custom
    Routes().get("/api/custom/<id:int>", [](Request& req) {
        int id = req.param("id").toInt();
        
        JsonDocument doc;
        doc["id"] = id;
        doc["timestamp"] = millis();
        doc["status"] = "success";
        
        return Response::json(doc);
    });
}
```

#### Parametry w URL

```
/api/user/<id:int>          → Liczba całkowita
/api/file/<path:path>       → Path (ze slashami)
/api/data/<name:string>     → String (bez slashów)
```

#### Response Codes

```cpp
Response::status(200)  // OK
Response::status(201)  // Created
Response::status(400)  // Bad Request
Response::status(404)  // Not Found
Response::status(500)  // Server Error
```

### 8.2 POST Request Handling

```cpp
Routes().post("/api/setting", [](Request& req) {
    // Parse JSON body
    JsonDocument doc;
    if (deserializeJson(doc, req.body) != DeserializationError::Ok) {
        JsonDocument error;
        error["error"] = "Invalid JSON";
        return Response::json(error).status(400);
    }
    
    // Extract values
    uint8_t value = doc["value"].as<uint8_t>();
    
    // Apply change
    SomeModule::setValue(value);
    
    // Return result
    JsonDocument result;
    result["status"] = "updated";
    result["value"] = value;
    return Response::json(result);
});
```

### 8.3 Middleware & Headers

```cpp
Routes().post("/api/protected", [](Request& req) {
    // Check header
    if (!req.header("X-API-Key").equals("secret")) {
        JsonDocument error;
        error["error"] = "Unauthorized";
        return Response::json(error).status(401);
    }
    
    // Add response header
    Response res = Response::json({...});
    res.setHeader("X-Custom-Header", "value");
    return res;
});
```

---

## 📡 SEKCJA IX: WEBSOCKET REAL-TIME EVENTS

### 9.1 Publikowanie Zdarzeń

Z dowolnego miejsca w kodzie:

```cpp
#include <web_integration.h>

// W EventBus listener
EventBus::getInstance().subscribe("UI", "some_event", [](auto data) {
    WebIntegration::getInstance().publishSensorData(
        "sensor_name",
        value,
        "unit"
    );
});
```

### 9.2 WebSocket JSON Format

Standaryzowany format dla wszystkich zdarzeń:

```json
{
    "event": "event_name",
    "timestamp": 123456789,
    "data": {
        "key": "value"
    },
    "source": "UI|HAL|SENSOR"
}
```

### 9.3 Client-Side Connection

```javascript
// Establish connection
const ws = new WebSocket('ws://192.168.1.100:81/ws/ui-events');

// Connection lifecycle
ws.onopen = () => console.log('Connected');
ws.onmessage = (event) => {
    const msg = JSON.parse(event.data);
    handleEvent(msg);
};
ws.onerror = (error) => console.error('Error:', error);
ws.onclose = () => {
    // Reconnect logic
    setTimeout(() => connectWebSocket(), 5000);
};

// Send command (server must implement)
function sendCommand(cmd) {
    ws.send(JSON.stringify({
        command: cmd,
        timestamp: Date.now()
    }));
}
```

---

## 📊 SEKCJA X: DEBUGGING & MONITORING

### 10.1 Serial Output

Sprawdzaj inicjalizację:

```
================================================
  ESP32-S3 LVGL + Web Framework Integration
================================================

[MAIN] Initializing WiFi...
[WebInt] Initializing WiFi...
...
[WebInt] WiFi connected!
[WebInt] IP Address: 192.168.1.100
[WebInt] Starting web server...
[WebInt] Web server started
[MAIN] Tasks created, scheduler running
```

### 10.2 Testowanie HTTP

```bash
# Status
curl http://192.168.1.100/

# System info
curl http://192.168.1.100/api/status

# POST data
curl -X POST http://192.168.1.100/api/brightness \
  -H "Content-Type: application/json" \
  -d '{"level": 80}'
```

### 10.3 WebSocket Testing

Użyj online tool: https://www.websocket.org/echo.html

Lub Python:

```python
import websocket
import json

ws = websocket.create_connection("ws://192.168.1.100:81/ws/ui-events")

while True:
    data = ws.recv()
    print(f"Received: {data}")
    msg = json.loads(data)
    print(f"Event: {msg['event']}")
```

### 10.4 Memory Monitoring

Serial monitor pokazuje:

```
[MAIN] Free Heap: 245024 bytes (74.8%)
[MAIN] Free PSRAM: 7680000 bytes (93.3%)
```

Jeśli spada poniżej 100KB - memory leak!

---

## 🔍 SEKCJA XI: COMMON TASKS

### 11.1 Dodaj Nowy Sensor

1. **Dodaj publikowanie w HAL:**

```cpp
// lib/YourSensor/YourSensor.h
class YourSensor {
public:
    float readValue();
};
```

2. **Dodaj do Web API:**

```cpp
Routes().get("/api/sensor/your-sensor", [](Request& req) {
    YourSensor sensor;
    float value = sensor.readValue();
    
    JsonDocument doc;
    doc["sensor"] = "your-sensor";
    doc["value"] = value;
    doc["unit"] = "°C";
    
    return Response::json(doc);
});
```

3. **Publikuj na WebSocket:**

```cpp
// W task lub timer
void sensorTask(void* param) {
    while (1) {
        float value = sensor.readValue();
        WebIntegration::getInstance().publishSensorData(
            "your-sensor",
            value,
            "°C"
        );
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
```

### 11.2 Implementuj Nowy Screen

Patrz oryginalny `DEVELOPMENT_GUIDE.md` sekcja I.

Dodatkowo, publikuj zmianę ekranu:

```cpp
// W ScreenManager::switchScreen()
EventBus::getInstance().publish("SCREEN", "changed", screenName);

// EventBus listener (web_integration.cpp)
EventBus::getInstance().subscribe("SCREEN", "changed", 
    [](String name) {
        WebIntegration::getInstance().publishScreenChange(name.c_str());
    }
);
```

### 11.3 Dodaj Ustawienia WiFi do UI

1. **Nowy Screen:**

```cpp
// ui/screens/wifi_settings_screen.h
class WiFiSettingsScreen : public ScreenBase {
public:
    WiFiSettingsScreen();
    void onInit() override;
    
private:
    static void apply_callback(lv_event_t * e);
};
```

2. **Obsługa:**

```cpp
static void apply_callback(lv_event_t * e) {
    // Get SSID from text input
    String ssid = lv_textarea_get_text(ta_ssid);
    String password = lv_textarea_get_text(ta_password);
    
    // Store in SPIFFS or EEPROM
    // Restart WiFi
    WiFi.disconnect();
    WebIntegration::getInstance().init(ssid.c_str(), password.c_str());
}
```

---

## ⚙️ SEKCJA XII: PERFORMANCE OPTIMIZATION

### 12.1 Task Priorities

```
Priority 2: LVGL Task     (UI responsiveness)
Priority 1: Default Task  (General)
Priority 1: WebServer Task (HTTP handling)
```

LVGL powinien być najwyższy - UI musi być smooth!

### 12.2 Memory Management

Limity:

```
Maximum JSON document: 1024 bytes
Maximum request size: 4096 bytes
WebSocket buffer: 2048 bytes
```

Optimizuj:

```cpp
// Zamiast
String json = "{...}";
broadcastMessage(channel, json.c_str());

// Użyj (zmniejsza kopie)
StaticJsonDocument<256> doc;
broadcastMessage(channel, doc.as<String>().c_str());
```

### 12.3 Task Stack Sizes

```cpp
xTaskCreatePinnedToCore(
    task_func,
    "Task Name",
    4096,  // ← Stack size w bytes
    NULL,
    priority,
    NULL,
    core
);

// Guidelines:
// LVGL:       8192  (wymaga sporo dla rendering)
// WebServer:  4096  (JSON, routing)
// Sensor:     2048  (prosty odczyt)
```

---

## 📋 CHECKLIST: Dodanie Nowej Funkcji WiFi

- [ ] Dodaj endpoint w `Routes().get()` lub `Routes().post()`
- [ ] Zdefiniuj JSON request/response format
- [ ] Dodaj event publikowanie (jeśli zmienia UI state)
- [ ] Testuj z `curl` lub Postman
- [ ] Dodaj WebSocket handler (jeśli real-time)
- [ ] Dokumentuj endpoint w `WEB_INTEGRATION.md`
- [ ] Sprawdź memory usage (Serial Monitor)
- [ ] Testuj na urządzeniu!

---

## 🐛 TROUBLESHOOTING

| Problem | Przyczyna | Rozwiązanie |
|---------|-----------|------------|
| **WiFi timeout** | SSID/pass błędne | Sprawdź `wifi_config.h` |
| **Port już zajęty** | Inny serwis na porcie 80 | Zmień HTTP_PORT w `wifi_config.h` |
| **JSON deserialize fail** | Zły format body | Sprawdź Content-Type i format |
| **WebSocket disconnect** | Keep-alive timeout | Implementuj reconnect w JS |
| **Memory heap low** | Wycieki zadań | Profile z `heaplog` |

---

## Linki Referencyjne

- [espwebframework README](../lib/espwebframework/README.md)
- [Original DEVELOPMENT_GUIDE](./DEVELOPMENT_GUIDE.md)
- [WiFi Integration Doc](./WEB_INTEGRATION.md)
- [VISION_INTEGRATED](./VISION_INTEGRATED.md)

