# ESP32 Web Framework

Django-inspired C++ web framework for ESP32-S3 with PSRAM support.

## Features

- **🔗 Django-like Router** - URL patterns with path parameters (`/user/<id:int>`)
- **📝 Template Engine** - Jinja2-inspired templates with variables, loops, conditions
- **🔌 WebSocket** - Real-time bidirectional communication with channels/rooms
- **⚙️ Middleware** - Request/response processing pipeline (CORS, Auth, Logging)
- **📊 ORM** - JSON-based data persistence with query interface
- **⏰ Scheduler** - Cron-like periodic task execution
- **📁 Static Files** - Automatic serving with caching and GZIP support
- **📤 File I/O** - Upload/download with multipart parser
- **🔍 Diagnostics** - Built-in system monitoring panel
- **💾 Cache** - PSRAM-optimized caching system

## Requirements

- ESP32-S3 with PSRAM (8MB recommended)
- PlatformIO
- Arduino Framework

## Quick Start

```cpp
#include <espwebframework.h>

using namespace espweb;

void setup() {
    // Quick setup with WiFi connection
    quickSetup("YourSSID", "YourPassword");
    
    // Define routes
    Routes().get("/", [](Request& req) {
        return Response::html("<h1>Hello ESP32!</h1>");
    });
    
    Routes().get("/api/status", [](Request& req) {
        JsonDocument doc;
        doc["uptime"] = millis() / 1000;
        doc["freeHeap"] = ESP.getFreeHeap();
        return Response::json(doc);
    });
}

void loop() {
    delay(100);
}
```

## Documentation

### Router

```cpp
// Simple routes
Routes().get("/", handler);
Routes().post("/submit", handler);
Routes().put("/update", handler);
Routes().del("/delete", handler);

// Path parameters
Routes().get("/user/<id:int>", [](Request& req) {
    int userId = req.param("id").toInt();
    // ...
});

Routes().get("/file/<path:path>", [](Request& req) {
    String filePath = req.param("path");
    // ...
});
```

### Templates

```cpp
// Load and render template
TemplateContext ctx;
ctx.set("title", "My Page");
ctx.set("items", itemsArray);

String html = Templates().render("page.html", ctx);
return Response::html(html);
```

Template syntax:
```html
<h1>{{ title }}</h1>

{% if user %}
    <p>Welcome, {{ user.name }}!</p>
{% endif %}

{% for item in items %}
    <li>{{ item.name }} - {{ item.price }}</li>
{% endfor %}

{% include "header.html" %}
```

### WebSocket

```cpp
class MyHandler : public WebSocketHandler {
    void onConnect(WebSocketClient* client) override {
        client->send("Welcome!");
    }
    
    void onMessage(WebSocketClient* client, const String& msg) override {
        Channels().broadcast("/ws/chat", msg);
    }
};

Channels().registerHandler("/ws/chat", std::make_shared<MyHandler>());
```

### Middleware

```cpp
// Built-in middleware
Server().use(std::make_shared<CORSMiddleware>());
Server().use(std::make_shared<LoggingMiddleware>());
Server().use(std::make_shared<AuthMiddleware>("secret-key"));

// Custom middleware
class MyMiddleware : public Middleware {
    bool processRequest(Request& req) override {
        // Return false to block request
        return true;
    }
    
    void processResponse(Request& req, Response& res) override {
        res.setHeader("X-Custom", "value");
    }
};
```

### Scheduler (Cron)

```cpp
// Schedule task every 30 seconds
Scheduler::getInstance().schedule("monitor", 30000, []() {
    LOG_INFO("Monitor", "Free heap: " + String(ESP.getFreeHeap()));
});

Scheduler::getInstance().start();
```

### ORM

```cpp
// Define model
class User : public Model {
public:
    Field<String> name{"name"};
    Field<String> email{"email"};
    Field<int> age{"age"};
};

// Create
User user;
user.name = "John";
user.email = "john@example.com";
user.save();

// Query
auto users = User::objects().filter("age", ">", 18).all();
```

### Cache

```cpp
// String cache
Caches().strings().set("key", "value", 60); // 60 sec TTL
String value = Caches().strings().get("key");

// Binary cache
Caches().binary().set("data", buffer, size);
```

## File Structure

```
lib/espwebframework/
├── src/
│   ├── espwebframework.h    # Main header
│   ├── settings.h           # Configuration
│   ├── core.hpp/cpp         # Server core
│   ├── router.hpp/cpp       # URL routing
│   ├── http.hpp/cpp         # Request/Response
│   ├── views.hpp/cpp        # View classes
│   ├── template.hpp/cpp     # Template engine
│   ├── middleware.hpp/cpp   # Middleware
│   ├── static.hpp/cpp       # Static files
│   ├── fileio.hpp/cpp       # File upload/download
│   ├── channels.hpp/cpp     # WebSocket
│   ├── orm.hpp/cpp          # Data persistence
│   ├── cron.hpp/cpp         # Task scheduler
│   ├── logger.hpp/cpp       # Logging
│   ├── diagnostics.hpp/cpp  # System monitoring
│   └── cache.hpp/cpp        # Caching
└── library.json

data/
├── templates/               # HTML templates
│   ├── index.html
│   └── ...
└── static/                  # Static files
    ├── css/
    ├── js/
    └── images/
```

## Configuration

Edit `settings.h` to customize:

```cpp
namespace settings {
    constexpr uint16_t SERVER_PORT = 80;
    constexpr uint16_t WS_PORT = 81;
    constexpr bool DEBUG_MODE = true;
    constexpr size_t PSRAM_ALLOCATION = 4 * 1024 * 1024;
    // ...
}
```

## License

MIT License
