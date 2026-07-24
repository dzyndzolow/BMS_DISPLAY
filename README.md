# 🚀 JC4827W543_Integrated

**ESP32-S3 LVGL Display + Web Framework Integration**

> Combined application featuring a professional 480x320 capacitive touch display with real-time web server, HTTP API, and WebSocket connectivity.

---

## ✨ Features

### Display & UI
- ✅ **LVGL 8.3** - Professional Graphics Library
- ✅ **480x320 IPS Display** (NV3041A driver)
- ✅ **Capacitive Touch** (GT911 controller)
- ✅ **Multi-Screen Navigation** with smooth transitions
- ✅ **Home, Settings, Demo Screens**

### Web & Connectivity
- ✅ **WiFi Support** (Station mode)
- ✅ **HTTP REST API** - Control display remotely
- ✅ **WebSocket** - Real-time bidirectional communication
- ✅ **JSON-based** - Easy integration with web/mobile apps
- ✅ **Django-like Routing** - Familiar endpoint structure

### System
- ✅ **FreeRTOS Multi-tasking** - LVGL + Web on separate cores
- ✅ **SD Card Support** - Storage and logging
- ✅ **Real-time Logging** - Debug and analytics
- ✅ **Memory Efficient** - ~50KB RAM, 1.2MB Flash code
- ✅ **OTA Ready** - Infrastructure for Over-The-Air updates

---

## 📋 Quick Start

### Prerequisites
- ESP32-S3 DevKit (8MB PSRAM recommended)
- USB Cable (COM7 for upload, COM8 for serial)
- Platform IO IDE or VS Code + PlatformIO extension

### 1. Clone & Setup

```bash
cd c:\
git clone <repo> JC4827W543_Integrated
cd JC4827W543_Integrated
```

### 2. Configure WiFi

Edit `include/wifi_config.h`:

```cpp
namespace WifiConfig {
    constexpr const char* SSID = "YourNetworkName";
    constexpr const char* PASSWORD = "YourPassword";
    constexpr bool ENABLE_WIFI_ON_BOOT = true;  // or false to disable
}
```

### 3. Build & Upload

**USB Upload:**
```bash
platformio run --target upload -e esp32-s3-devkitc1-n4r8-usb
```

**Monitor:**
```bash
platformio device monitor -b 115200 -p COM8
```

### 4. Access Device

**Serial Monitor:**
```
================================================
  ESP32-S3 LVGL + Web Framework Integration
================================================

[MAIN] Initializing WiFi...
[WebInt] WiFi connected!
[WebInt] IP Address: 192.168.1.100
[WebInt] Web server started
```

**Web Browser:**
- Status: `http://192.168.1.100/`
- API: `http://192.168.1.100/api/status`

**WebSocket:**
```javascript
const ws = new WebSocket('ws://192.168.1.100:81/ws/ui-events');
ws.onmessage = (e) => console.log(JSON.parse(e.data));
```

---

## 🏗️ Architecture

### Directory Structure

```
JC4827W543_Integrated/
├── include/                    # Headers
│   ├── display.h              # Display HAL
│   ├── touch_input.h          # Touch controller
│   ├── wifi_config.h          # WiFi configuration
│   ├── web_integration.h      # NEW - Web server integration
│   ├── lv_conf.h              # LVGL configuration
│   ├── logger.h               # Logging system
│   └── ...
│
├── src/
│   ├── main.cpp               # UPDATED - WiFi + Web support
│   ├── core/                  # Boot & initialization
│   ├── hal/                   # Hardware abstraction
│   ├── ui/                    # LVGL screens & widgets
│   ├── services/              # EventBus, Logger, etc.
│   ├── integration/
│   │   └── web_integration.cpp  # NEW - Web framework glue
│   └── ...
│
├── lib/
│   ├── espwebframework/       # NEW - HTTP/WebSocket framework
│   ├── SD_Manager/            # SD card operations
│   ├── Touch_GT911/           # Touch controller driver
│   └── README
│
├── docs/
│   ├── VISION.md              # LVGL system philosophy
│   ├── VISION_INTEGRATED.md   # NEW - Combined system vision
│   ├── DEVELOPMENT_GUIDE.md   # LVGL development guide
│   ├── DEVELOPMENT_WEB_INTEGRATION.md  # NEW - Web dev guide
│   ├── WEB_INTEGRATION.md     # NEW - API documentation
│   └── ...
│
├── platformio.ini             # UPDATED - Both frameworks
├── custom_partitions.csv      # Memory partitioning
└── README.md                  # THIS FILE
```

---

## 🌐 Web API Reference

### Status Endpoints

```bash
# Health check
GET /
# Response: {"status": "online", "device": "ESP32-S3 LVGL System", ...}

# System metrics
GET /api/status
# Response: {"uptime": 3600, "freeHeap": 245024, "cpuTemp": 45}

# Current screen info
GET /api/screen
# Response: {"currentScreen": "home", "availableScreens": [...]}
```

### Control Endpoints

```bash
# Change brightness
POST /api/brightness
# Body: {"level": 80}
# Response: {"brightness": 80, "status": "updated"}

# Get/set configuration
GET /api/config
POST /api/config
```

### WebSocket Channels

```
ws://192.168.1.100:81/ws/ui-events
   → Screen changes
   → Button clicks
   → Settings updates

ws://192.168.1.100:81/ws/sensors
   → Temperature readings
   → Humidity, Pressure, etc.
   → Real-time updates
```

---

## 📡 WebSocket Event Format

All events use standardized JSON:

```json
{
    "event": "screen_changed",
    "timestamp": 1702053600000,
    "data": {
        "screen": "settings",
        "transition": "fade"
    }
}
```

---

## 🔧 Development

### Adding a New REST Endpoint

Edit `src/integration/web_integration.cpp`:

```cpp
Routes().post("/api/custom", [](Request& req) {
    JsonDocument doc;
    deserializeJson(doc, req.body);
    
    // Process request
    uint8_t value = doc["value"].as<uint8_t>();
    
    // Return response
    JsonDocument res;
    res["status"] = "success";
    res["value"] = value;
    return Response::json(res);
});
```

### Adding a New UI Screen

See `docs/DEVELOPMENT_GUIDE.md` Section I.

Then, publish screen change event:

```cpp
EventBus::getInstance().publish("SCREEN", "changed", "my_screen");
```

### Publishing Real-Time Data

```cpp
#include <web_integration.h>

// In any task/callback
WebIntegration::getInstance().publishSensorData(
    "temperature",
    45.2,
    "°C"
);
```

---

## 🐛 Debugging

### Serial Monitor Output

```bash
platformio device monitor -b 115200 -p COM8

# Expected output:
[MAIN] SD Card initialized
[MAIN] Initializing WiFi...
[WebInt] WiFi connected!
[WebInt] IP Address: 192.168.1.100
[MAIN] Tasks created, scheduler running
```

### Test HTTP Endpoints

```bash
# Get status
curl http://192.168.1.100/api/status

# Control brightness
curl -X POST http://192.168.1.100/api/brightness \
  -H "Content-Type: application/json" \
  -d '{"level": 80}'
```

### Memory Profiling

Monitor in Serial output:
```
Free Heap: 245024 bytes (74.8%)
Free PSRAM: 7680000 bytes (93.3%)
```

If heap drops below 100KB → investigate memory leaks.

---

## 🎯 Common Tasks

| Task | File | Reference |
|------|------|-----------|
| Modify display pins | `include/defaults.h` or `include/display.h` | DEVELOPMENT_GUIDE.md |
| Add WiFi network | `include/wifi_config.h` | WEB_INTEGRATION.md |
| Create new screen | `src/ui/screens/` | DEVELOPMENT_GUIDE.md Sec. I |
| Add HTTP endpoint | `src/integration/web_integration.cpp` | DEVELOPMENT_WEB_INTEGRATION.md |
| Subscribe to events | `src/services/` | DEVELOPMENT_GUIDE.md Sec. III |
| Configure LVGL | `include/lv_conf.h` | LVGL documentation |

---

## 📦 Dependencies

- **PlatformIO Core** - Build system
- **Arduino Framework** - Core libraries
- **LVGL 8.3.11** - Graphics library
- **GFX Library for Arduino** - Display driver
- **ArduinoJson 7.0+** - JSON parsing
- **ArduinoWebsockets 0.5.3** - WebSocket client

All configured in `platformio.ini`.

---

## 🚀 Advanced Features

### OTA Updates
Infrastructure prepared, needs implementation.

### HTTPS/SSL
Requires certificate storage on SPIFFS.

### Mobile App Integration
REST API compatible with any HTTP client.

### Data Logging
EventBus events can be logged to SD card for analytics.

---

## 📚 Documentation

- **[VISION.md](docs/VISION.md)** - System philosophy (LVGL)
- **[VISION_INTEGRATED.md](docs/VISION_INTEGRATED.md)** - Full system vision
- **[DEVELOPMENT_GUIDE.md](docs/DEVELOPMENT_GUIDE.md)** - LVGL development
- **[DEVELOPMENT_WEB_INTEGRATION.md](docs/DEVELOPMENT_WEB_INTEGRATION.md)** - Web development
- **[WEB_INTEGRATION.md](docs/WEB_INTEGRATION.md)** - API reference

---

## 🤝 Contributing

When adding new features:

1. Follow architecture in `VISION_INTEGRATED.md`
2. Use EventBus for inter-module communication
3. Separate concerns (UI ≠ Network ≠ HAL)
4. Add WebSocket events for real-time data
5. Document endpoints in `WEB_INTEGRATION.md`
6. Test memory usage under load

---

## 🐞 Troubleshooting

### WiFi Won't Connect
1. Check SSID/password in `wifi_config.h`
2. Verify router is 2.4GHz (ESP32-S3 limitation)
3. Check serial output for error codes

### HTTP Endpoints Timeout
1. Verify IP address is correct
2. Check firewall isn't blocking ports 80/81
3. Ensure WebIntegration::start() was called

### WebSocket Disconnects
1. Implement client-side reconnect logic
2. Check PSRAM availability
3. Monitor server-side task stack usage

### Out of Memory
1. Check for memory leaks in tasks
2. Reduce buffer sizes in config
3. Profile with `heaplog` 

---

## 📞 Support

For issues or questions:
1. Check relevant documentation in `docs/`
2. Review Serial Monitor output for errors
3. Test individual components (LVGL, WiFi) separately

---

## 📄 License

[Your License Here]

---

**Last Updated:** 8 December 2024  
**Current Version:** 1.0.0  
**Status:** Active Development
