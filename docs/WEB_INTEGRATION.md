# 🔗 WiFi & Web Framework Integration

> **Dodane:** 8 grudnia 2024  
> **Status:** Dokumentacja integracji  
> **Wersja:** 1.0.0

---

## 📋 Spis Treści

1. [Architektura Integracji](#architektura-integracji)
2. [Komponenty WebFramework](#komponenty-webframework)
3. [Konfiguracja WiFi](#konfiguracja-wifi)
4. [API Endpoints](#api-endpoints)
5. [WebSocket Events](#websocket-events)
6. [Rozszerzone Możliwości](#rozszerzone-możliwości)
7. [Troubleshooting](#troubleshooting)

---

## Architektura Integracji

### Warstwa Integracji

```
┌─────────────────────────────────────────────────────┐
│           LVGL UI (480x320 Touch)                   │
│        - Home Screen, Settings, Demo                │
│        - Touch Input Handler                        │
│        - Screen Manager                             │
└────────────────────────┬────────────────────────────┘
                         │
                    EventBus
                    (Pub/Sub)
                         │
    ┌────────────────────┼────────────────────┐
    │                    │                    │
    ▼                    ▼                    ▼
┌─────────┐      ┌──────────────┐    ┌──────────────┐
│   HAL   │      │  Services    │    │ Web Framework│
│ Layer   │      │  (Logger,    │    │  Integration │
│ ─────── │      │   EventBus)  │    │ (WebServer)  │
│Display  │      │              │    │              │
│Touch    │      │              │    │ HTTP Routes  │
│Storage  │      │              │    │ WebSocket    │
└────┬────┘      └──────────────┘    │ Channels     │
     │                                 └──────┬───────┘
     │                                        │
     └────────────┬───────────────────────────┘
          Hardware Interface
          (Pin Control, GPIO)
```

---

## Komponenty WebFramework

### WebIntegration Singleton

**Plik:** `include/web_integration.h`

Główny komponent zarządzający komunikacją między LVGL a Web Framework.

```cpp
// Singleton accessor
WebIntegration& webServer = WebIntegration::getInstance();

// Inicjalizacja
webServer.init("SSID", "PASSWORD", 80);
webServer.start();

// Publikowanie zdarzeń
webServer.publishBrightnessChange(100);
webServer.publishScreenChange("home");
webServer.publishSensorData("temperature", 45.2, "°C");

// Broadcast do klientów
webServer.broadcastMessage("/ws/ui-events", "{...}");
```

### WiFi Config

**Plik:** `include/wifi_config.h`

Single Source of Truth dla konfiguracji WiFi.

```cpp
namespace WifiConfig {
    constexpr const char* SSID = "your-network";
    constexpr const char* PASSWORD = "password";
    constexpr uint16_t HTTP_PORT = 80;
    constexpr bool ENABLE_WIFI_ON_BOOT = true;
}
```

---

## Konfiguracja WiFi

### Włączanie/Wyłączanie WiFi

W `wifi_config.h`:

```cpp
// Włącz WiFi w boot
constexpr bool ENABLE_WIFI_ON_BOOT = true;

// Zmień SSID i hasło
constexpr const char* SSID = "YourNetworkName";
constexpr const char* PASSWORD = "YourPassword";
```

### Proces Inicjalizacji

1. **Serial Port** - 115200 baud
2. **SD Card** - Inicjalizacja i logger
3. **LVGL Display** - Display i touch input
4. **WiFi (opcjonalnie)** - Połączenie z siecią
5. **Web Server** - Uruchamianie routów HTTP
6. **FreeRTOS Tasks** - LVGL, Default, WebServer

```
setup() {
    Serial.begin(115200);
    sd_card_init();
    logger_init();
    
    // WiFi - optional
    if (ENABLE_WIFI_ON_BOOT) {
        WebIntegration::getInstance().init(...);
        WebIntegration::getInstance().start();
    }
    
    // Tasks
    xTaskCreatePinnedToCore(lvglTask, ...);
    xTaskCreatePinnedToCore(defaultTask, ...);
    
    // Web server task (if WiFi enabled)
    if (wifiEnabled) {
        xTaskCreatePinnedToCore(webServerTask, ...);
    }
}
```

---

## API Endpoints

### Status Endpoints

#### GET `/`
Status strony głównej

```bash
curl http://192.168.1.100/
```

Response:
```json
{
  "status": "online",
  "device": "ESP32-S3 LVGL System",
  "uptime": 3600,
  "freeHeap": 245024,
  "freePSRAM": 7680000
}
```

#### GET `/api/status`
Szczegółowe informacje systemowe

```bash
curl http://192.168.1.100/api/status
```

Response:
```json
{
  "uptime": 3600,
  "freeHeap": 245024,
  "freePSRAM": 7680000,
  "cpuTemp": 45
}
```

### Control Endpoints

#### POST `/api/brightness`
Zmiana jasności wyświetlacza

```bash
curl -X POST http://192.168.1.100/api/brightness \
  -H "Content-Type: application/json" \
  -d '{"level": 80}'
```

Response:
```json
{
  "brightness": 80,
  "status": "updated"
}
```

#### GET `/api/screen`
Informacje o bieżącym ekranie

```bash
curl http://192.168.1.100/api/screen
```

Response:
```json
{
  "currentScreen": "home",
  "availableScreens": ["home", "settings", "demo"]
}
```

---

## WebSocket Events

### Kanały WebSocket

#### `/ws/ui-events`
Zdarzenia z interfejsu LVGL

```json
// Zmiana ekranu
{
  "event": "screen_changed",
  "screen": "settings",
  "timestamp": 12345678
}

// Zmiana jasności
{
  "event": "brightness_changed",
  "value": 75,
  "timestamp": 12345678
}

// Klik buttona
{
  "event": "button_pressed",
  "buttonId": "home_settings",
  "timestamp": 12345678
}
```

#### `/ws/sensors`
Dane z sensorów

```json
{
  "event": "sensor_data",
  "sensor": "cpu_temp",
  "value": 45.2,
  "unit": "°C",
  "timestamp": 12345678
}
```

### Client-Side Example (JavaScript)

```javascript
// Połączenie do WebSocket
const ws = new WebSocket('ws://192.168.1.100:81/ws/ui-events');

ws.onopen = (event) => {
    console.log('Connected to device');
};

ws.onmessage = (event) => {
    const data = JSON.parse(event.data);
    console.log('Event received:', data);
    
    if (data.event === 'screen_changed') {
        updateUI(data.screen);
    }
    if (data.event === 'brightness_changed') {
        updateBrightnessDisplay(data.value);
    }
};

ws.onerror = (error) => {
    console.error('WebSocket error:', error);
};
```

---

## Rozszerzone Możliwości

### Dodawanie Nowych Endpoint API

W `web_integration.cpp`, w metodzie `start()`:

```cpp
// Nowy endpoint
Routes().get("/api/custom/<param:string>", [](Request& req) {
    String param = req.param("param");
    
    JsonDocument doc;
    doc["received"] = param;
    doc["status"] = "success";
    
    return Response::json(doc);
});
```

### Publikowanie Zdarzeń z UI

Gdy się coś dzieje w LVGL, publikuj do WebSocket:

```cpp
// W obsłudze buttona (np. ui/screens/home_screen.cpp)
#include <web_integration.h>

static void button_callback(lv_event_t * e) {
    // Obsługa buttona w LVGL
    doSomething();
    
    // Publikuj zdarzenie
    WebIntegration::getInstance().publishScreenChange("new_screen");
}
```

### Integracja z Middleware

```cpp
// Dodanie middleware do web serwera
class AuthMiddleware : public Middleware {
    bool processRequest(Request& req) override {
        // Weryfikacja API key
        if (!req.header("X-API-Key").equals("secret")) {
            return false;  // Zablokuj żądanie
        }
        return true;
    }
};

Server().use(std::make_shared<AuthMiddleware>());
```

---

## Troubleshooting

### WiFi nie Łączy się

1. **Sprawdź SSID i hasło** w `wifi_config.h`
2. **Serial Monitor** - obserwuj komunikaty debug
3. **RSSI Signal** - sprawdź siłę sygnału WiFi

```
[WebInt] Initializing WiFi...
.......
[WebInt] WiFi connected!
[WebInt] IP Address: 192.168.1.100
```

### Brak Odpowiedzi HTTP

1. **Sprawdź IP adres** - `http://<IP>:<PORT>/`
2. **Monitor portów** - czy Web Framework nasłuchuje
3. **Firewall** - może blokować porty

### WebSocket Disconnect

1. **Reconnection logic** - zaimplementuj w client-side
2. **Keep-alive** - sprawdzaj connection status
3. **Log messages** - monitoruj WebSocket kanały

### Memory Issues

Monitor w serial monitor:

```
[MAIN] Free Heap: 245024 bytes
[MAIN] Free PSRAM: 7680000 bytes
```

Jeśli heap spada:
- Zwiększ `webServerTask` stack size w `main.cpp`
- Zmniejsz rozmiar buffora WebSocket
- Optymalizuj obsługę zdarzeń

---

## Checklist Integracji

- [ ] WiFi skonfigurowany w `wifi_config.h`
- [ ] LVGL zadania działają (ekran wyświetla się)
- [ ] Web serwer nasłuchuje na porcie 80
- [ ] Testy HTTP endpoints z curl/Postman
- [ ] WebSocket loguje się prawidłowo
- [ ] Zdarzenia LVGL publikują się do klientów
- [ ] Memory usage monitorowany w czasie rzeczywistym

---

## Następne Kroki

1. **Dodaj Authentication** - zabezpiecz API
2. **HTTPS Support** - SSL certificates
3. **OTA Updates** - Over-The-Air programowanie
4. **Data Logging** - store events na SPIFFS
5. **Mobile App** - dedykowana aplikacja mobilna

