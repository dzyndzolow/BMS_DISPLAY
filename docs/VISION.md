# 🎯 WIZJA ARCHITEKTURY - Koncepcja Projektu

> **Projekt:** ESP32-S3 LVGL System (JC4827W543)  
> **Data utworzenia:** 8 grudnia 2024  
> **Filozofia:** Bezkompromisowa jakość kodu embedded

---

## 💡 GŁÓWNA KONCEPCJA

### Motto projektu
**"Zero Technical Debt - Build Once, Scale Forever"**

Tworzymy system embedded, który:
- Jest **czytelny** dla AI i człowieka
- Jest **modularny** - każdy komponent wymienialny
- Jest **testowalny** - każda warstwa niezależna
- Jest **skalowalny** - łatwo dodać WiFi, OTA, sensory
- Jest **maintainable** - kod nie gnije z czasem

---

## 🏛️ FUNDAMENTALNE ZASADY

### 1. Single Source of Truth
**Problem:** 5+ plików z hardkodowanymi pinami, 3 miejsca inicjalizacji LVGL.

**Rozwiązanie:** `config/defaults.h` - **JEDNO** źródło prawdy dla wszystkich stałych.

```cpp
namespace DisplayConfig {
    constexpr uint8_t PIN_BL = 1;  // ← Zmiana w JEDNYM miejscu
}
```

Każdy moduł odwołuje się do `DisplayConfig::PIN_BL`. Zero duplikacji.

---

### 2. Separation of Concerns (Rozdzielenie Odpowiedzialności)

**Koncepcja:** Każda warstwa robi TYLKO swoją pracę.

```
┌─────────────────────────────────────────────────┐
│  UI Layer        │ ScreenManager, HomeScreen    │  ← Prezentacja
│                  │ Tylko LVGL, zero hardware    │
├─────────────────────────────────────────────────┤
│  Services Layer  │ EventBus, Logger             │  ← Logika biznesowa
│                  │ Komunikacja między warstwami │
├─────────────────────────────────────────────────┤
│  HAL Layer       │ IDisplay, ITouch, IStorage   │  ← Abstrakcja hardware
│                  │ Interfejsy + implementacje   │
├─────────────────────────────────────────────────┤
│  Core Layer      │ boot.cpp, tasks_new.cpp      │  ← Lifecycle systemu
│                  │ Unified boot sequence        │
└─────────────────────────────────────────────────┘
```

**Zasada:** UI nigdy nie wywołuje bezpośrednio hardware. Tylko przez HAL + EventBus.

---

### 3. Interface-Based Design (Projektowanie przez interfejsy)

**Problem:** Kod ściśle sprzężony z konkretnym hardware (NV3041A, GT911).

**Rozwiązanie:** Interfejsy HAL.

```cpp
/* Interfejs - umowa */
class IDisplay {
public:
    virtual bool init() = 0;
    virtual void setBrightness(uint8_t percent) = 0;
    virtual void flush(...) = 0;
};

/* Implementacja konkretna */
class NV3041ADisplay : public IDisplay {
    bool init() override { /* specyfika NV3041A */ }
};
```

**Korzyść:** Zmiana display na inny? Napisz nową implementację `IDisplay`. UI ani bajt kodu nie zmienisz.

---

### 4. Event-Driven Architecture (Architektura zdarzeniowa)

**Problem:** Moduły bezpośrednio się wywołują - ścisłe sprzężenie.

**Rozwiązanie:** EventBus - Publish/Subscribe.

```
┌──────────────┐                  ┌──────────────┐
│ SettingsScreen│                 │  StatusBar   │
│              │                  │              │
│ setBrightness│                  │              │
└──────┬───────┘                  └──────▲───────┘
       │                                 │
       │ PUBLISH(BRIGHTNESS_CHANGE, 75) │
       │                                 │
       └────────────►EventBus◄───────────┘
                         │
                    BROADCAST
```

**Korzyść:** SettingsScreen nie wie o StatusBar. Luźne wiązanie = łatwa rozbudowa.

---

### 5. Unified Boot Sequence (Pojedyncza sekwencja startowa)

**Problem:** Inicjalizacja rozrzucona po 3 plikach. Nieznana kolejność, duplikacje, undefined behavior.

**Rozwiązanie:** `Core::boot()` - deterministyczne 8 etapów.

```cpp
BootStage::SERIAL_INIT    →
BootStage::STORAGE_INIT   →
BootStage::LOGGER_INIT    →
BootStage::DISPLAY_INIT   →
BootStage::TOUCH_INIT     →
BootStage::LVGL_INIT      →
BootStage::UI_INIT        →
BootStage::COMPLETE       ✓
```

**Korzyść:** Wiesz DOKŁADNIE co, kiedy i w jakiej kolejności się inicjalizuje. Debugowanie błędów boot = 10x szybsze.

---

### 6. Dependency Injection (Wstrzykiwanie zależności)

**Problem:** God Objects - klasa wie o wszystkim, tworzy wszystko wewnątrz.

**Rozwiązanie:** Przekazuj zależności z zewnątrz.

```cpp
/* ŹLE - God Object */
class Display {
    void init() {
        _spi = new SPIMaster();  // ← Tworzy wewnątrz!
        _spi->begin();
    }
};

/* DOBRZE - Dependency Injection */
class Display {
    Display(ISPIMaster* spi) : _spi(spi) {}  // ← Dostaje z zewnątrz
    
    void init() {
        _spi->begin();
    }
private:
    ISPIMaster* _spi;
};
```

**Korzyść:** Testowanie - możesz przekazać mock SPI. Brak ukrytych zależności.

---

### 7. RAII (Resource Acquisition Is Initialization)

**Koncepcja:** Konstruktor alokuje zasoby, destruktor zwalnia. ZAWSZE.

```cpp
class NV3041ADisplay {
public:
    NV3041ADisplay() {
        _bus = new Arduino_ESP32QSPI(...);  // ← Alokuj w konstruktorze
    }
    
    ~NV3041ADisplay() {
        delete _bus;  // ← Zwolnij w destruktorze
        _bus = nullptr;
    }
};
```

**Korzyść:** Zero memory leaks. Jeśli obiekt istnieje, zasoby są OK. Jeśli został usunięty, wszystko zwolnione.

---

### 8. Const Correctness (Poprawność stałości)

**Zasada:** Jeśli metoda NIE ZMIENIA stanu, oznacz `const`.

```cpp
class Display {
    uint16_t getWidth() const { return _width; }    // ← const!
    uint8_t getBrightness() const { return _br; }   // ← const!
    
    void setBrightness(uint8_t br) { _br = br; }    // ← NIE const
};
```

**Korzyść:** Kompilator WYMUSZA że metoda const nic nie zmienia. Bezpieczeństwo w compile-time.

---

## 🔧 WZORCE PROJEKTOWE

### Singleton (EventBus, ScreenManager)

**Kiedy:** Potrzebujesz dokładnie JEDNEJ instancji w całym systemie.

```cpp
class EventBus {
public:
    static EventBus& instance() {
        static EventBus inst;  // ← Meyers Singleton
        return inst;
    }
private:
    EventBus() {}  // ← Prywatny konstruktor
};

// Użycie
EVENT_BUS.publish(...);
```

---

### Observer Pattern (EventBus Pub/Sub)

**Kiedy:** Wiele obiektów chce reagować na to samo zdarzenie.

```
Publisher ─────► EventBus ─────► Observer 1
                    │
                    ├─────────► Observer 2
                    │
                    └─────────► Observer 3
```

---

### Strategy Pattern (HAL Interfaces)

**Kiedy:** Ta sama operacja, różne implementacje (np. różne displaye).

```cpp
IDisplay* display;

if (hardware == "NV3041A") {
    display = new NV3041ADisplay();
} else if (hardware == "ST7789") {
    display = new ST7789Display();
}

display->init();  // ← Ten sam interfejs, inna implementacja
```

---

### Factory Pattern (Screen creation)

**Kiedy:** Tworzenie obiektów na podstawie typu.

```cpp
ScreenBase* ScreenFactory::create(ScreenId id) {
    switch(id) {
        case ScreenId::HOME:     return new HomeScreen();
        case ScreenId::SETTINGS: return new SettingsScreen();
        default:                 return nullptr;
    }
}
```

---

## 🎨 ZASADY CZYTELNOŚCI KODU

### 1. Małe pliki (max 300 linii)
Jeden plik = jedna odpowiedzialność. Jeśli plik rośnie > 300 linii, dziel na mniejsze.

### 2. Małe funkcje (max 50 linii)
Funkcja robi JEDNĄ rzecz. Jeśli > 50 linii, wydziel pomocnicze funkcje.

### 3. Mówiące nazwy
```cpp
/* ŹLE */
int d;  // elapsed time in days

/* DOBRZE */
int elapsedTimeInDays;
```

### 4. Namespace per warstwa
```cpp
namespace HAL { ... }
namespace UI { ... }
namespace Services { ... }
namespace Core { ... }
```

Nie ma globalnej przestrzeni nazw. Wszystko w namespace.

### 5. Dokumentacja Doxygen
```cpp
/**
 * @brief Initialize display hardware
 * @return true on success, false on failure
 */
bool init();
```

Każda publiczna metoda ma komentarz Doxygen.

---

## 🧪 ZASADY TESTOWALNOŚCI

### 1. Interfejsy wszędzie
Każda zależność przez interfejs = możesz podmienić na mock.

```cpp
/* Testowanie display bez hardware */
class MockDisplay : public IDisplay {
    bool init() override { return true; }  // ← Zawsze sukces
};
```

### 2. Brak side effects
Funkcja nie modyfikuje globalnego stanu. Tylko co dostanie na wejściu.

### 3. Deterministyczne zachowanie
Ta sama konfiguracja = ten sam wynik. Zawsze.

---

## 🚀 POSTĘPOWANIE W PROJEKCIE

### Faza 0: Centralna Konfiguracja
**Cel:** Jedna prawda o hardware.

**Rezultat:** `config/defaults.h` z namespace'ami dla każdego modułu.

---

### Faza 1: HAL (Hardware Abstraction Layer)
**Cel:** Oddzielić logikę od hardware.

**Rezultat:** 
- `IDisplay.h`, `ITouch.h`, `IStorage.h` - interfejsy
- `NV3041ADisplay`, `GT911Touch` - implementacje

**Korzyść:** Zmiana hardware = nowa implementacja, zero zmian w UI.

---

### Faza 2: Unified Boot
**Cel:** Jedna deterministyczna sekwencja startowa.

**Rezultat:** `Core::boot()` eliminuje duplikacje init.

**Korzyść:** Boot sequence jest PRZEJRZYSTY i łatwy do debugowania.

---

### Faza 3: EventBus
**Cel:** Luźne wiązanie między modułami.

**Rezultat:** `Services::EventBus` - Pub/Sub na enum EventType.

**Korzyść:** Moduły nie znają się nawzajem. Łatwa rozbudowa.

---

### Faza 4: UI Modularyzacja
**Cel:** Ekrany jako niezależne komponenty.

**Rezultat:**
- `ScreenBase` - klasa bazowa z lifecycle
- `ScreenManager` - nawigacja + animacje
- `HomeScreen` - przykład

**Korzyść:** Dodanie nowego ekranu = 1 plik `.cpp/.h`. Zero zmian w istniejącym kodzie.

---

## 📐 PORÓWNANIE: PRZED vs PO

| Aspekt | PRZED | PO |
|--------|-------|-----|
| **Inicjalizacja LVGL** | 3 miejsca (tasks.cpp, display.cpp, ui_init.cpp) | 1 miejsce (boot.cpp) |
| **Piny GPIO** | 5+ plików | 1 plik (defaults.h) |
| **Globalne zmienne** | Tak (touch_globals.cpp) | Nie (wszystko w klasach) |
| **Coupling** | Wysoki (bezpośrednie wywołania) | Niski (EventBus) |
| **Testowalność** | Niemożliwa (brak interfejsów) | Wysoka (HAL interfaces) |
| **Czytelność** | Niska (God Objects) | Wysoka (małe klasy) |
| **Rozbudowa** | Trudna (modyfikacja istniejącego) | Łatwa (dodanie nowego) |

---

## 🎓 INSPIRACJE I ŹRÓDŁA

### 1. Clean Architecture (Robert C. Martin)
- Separacja warstw
- Dependency Rule (zależności tylko w jednym kierunku)
- Interface Segregation

### 2. SOLID Principles
- **S**ingle Responsibility
- **O**pen/Closed (otwarte na rozszerzenia, zamknięte na modyfikacje)
- **L**iskov Substitution
- **I**nterface Segregation
- **D**ependency Inversion

### 3. Embedded Best Practices (ESP-IDF)
- HAL pattern
- FreeRTOS task management
- Event-driven architecture

### 4. LVGL Porting Guide
- Display/Input callbacks
- Buffer management
- Thread safety

---

## 💭 FILOZOFIA KODU

### "Kod piszemy raz, czytamy 100 razy"
Priorytet: **czytelność** > wydajność (o ile wydajność wystarczająca).

### "Jeśli nie możesz tego wytłumaczyć AI, to za skomplikowane"
Test: czy GitHub Copilot zrozumie intencję funkcji?

### "Zero Technical Debt"
Nie odkładamy refactoringu na później. Robimy dobrze od początku.

### "Make it work, make it right, make it fast"
1. Najpierw działający kod
2. Potem refactor (clean code)
3. Na końcu optymalizacja (jeśli potrzebna)

---

## 🔮 DŁUGOTERMINOWA WIZJA

Ten kod ma być **fundamentem** na lata. 

**Przyszłość (bez modyfikacji core):**
- Dodanie WiFi → nowy moduł w `services/`, komunikacja przez EventBus
- Dodanie OTA → subskrybuje `WIFI_CONNECTED`, publikuje `OTA_PROGRESS`
- Nowy sensor → implementacja `ISensor`, rejestracja w boot
- Nowy ekran → dziedziczy `ScreenBase`, rejestracja w ScreenManager

**Wszystko przez:**
- Interfejsy (HAL)
- EventBus (komunikacja)
- Dependency Injection (luźne wiązanie)

**Efekt:** Kod nie gnije. Dodajesz, nie modyfikujesz.

---

## ✅ PODSUMOWANIE KONCEPCJI

**Cel osiągnięty:** System embedded klasy profesjonalnej, gotowy na lata rozbudowy.

**Główne filary:**
1. ✅ **Single Source of Truth** - config/defaults.h
2. ✅ **HAL** - abstrakcja hardware przez interfejsy
3. ✅ **EventBus** - luźne wiązanie modułów
4. ✅ **Unified Boot** - deterministyczna inicjalizacja
5. ✅ **Screen Management** - modularny UI
6. ✅ **Clean Code** - czytelność i maintainability

**Rezultat:**
- Zero duplikacji
- Łatwe testowanie
- Szybka rozbudowa
- Współpraca AI/człowiek

---

**"Build Once, Scale Forever"** ✨
