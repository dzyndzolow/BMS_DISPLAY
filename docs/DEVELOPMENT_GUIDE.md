# 📚 PRZEWODNIK PROGRAMISTY - ESP32-S3 LVGL System

> **Wersja:** 1.0.0  
> **Data:** 8 grudnia 2024  
> **Status:** Dokumentacja referencyjna

---

## 📖 WPROWADZENIE

### Cel dokumentu

Ten przewodnik jest **kompletnym źródłem informacji** dla programistów rozwijających system ESP32-S3 LVGL. Zawiera:

- Szczegółowe instrukcje dodawania nowych komponentów
- Zasady komunikacji między modułami
- Wzorce projektowe i best practices
- Kompletne przykłady kodu z komentarzami
- Checklist weryfikacyjne dla każdego typu zmian

### Dla kogo jest ten dokument?

- Programistów dodających nowe funkcjonalności
- Osób utrzymujących i debugujących system
- AI asystentów (GitHub Copilot, Claude) wspomagających rozwój
- Code reviewers weryfikujących pull requesty

---

## 📊 STAN OBECNY SYSTEMU

### Architektura systemu

System zbudowany jest w oparciu o **warstwową architekturę** z jasno określonymi granicami i odpowiedzialnościami:

```
┌────────────────────────────────────────────────────────────┐
│                     APPLICATION LAYER                       │
│                    (LvglWidgets.cpp)                        │
│                      Entry Point                            │
└────────────────────┬───────────────────────────────────────┘
                     │
┌────────────────────▼───────────────────────────────────────┐
│                      CORE LAYER                             │
│  • boot.cpp         - Unified initialization sequence       │
│  • tasks_new.cpp    - FreeRTOS task management             │
│  • main_new.cpp     - Alternative entry point              │
└────────────────────┬───────────────────────────────────────┘
                     │
    ┌────────────────┼────────────────┐
    │                │                │
┌───▼──────────┐ ┌──▼──────────┐ ┌──▼──────────────┐
│  HAL LAYER   │ │ SERVICES    │ │   UI LAYER      │
│              │ │   LAYER     │ │                 │
│ • IDisplay   │ │ • EventBus  │ │ • ScreenMgr     │
│ • ITouch     │ │ • Logger    │ │ • ScreenBase    │
│ • IStorage   │ │             │ │ • HomeScreen    │
│              │ │             │ │                 │
│ NV3041A impl │ │ Pub/Sub     │ │ Navigation +    │
│ GT911 impl   │ │ Pattern     │ │ Animations      │
└──────────────┘ └─────────────┘ └─────────────────┘
```

### Metryki systemu

| Zasób | Zużycie | Limit | Ocena |
|-------|---------|-------|-------|
| **RAM** | 32,148 B (9.8%) | 327,680 B | ✅ Doskonały |
| **Flash** | 810,343 B (19.3%) | 4,194,304 B | ✅ Doskonały |
| **Pliki źródłowe** | ~35 plików | - | ✅ Modularny |
| **Avg. linie/plik** | ~150 linii | < 300 | ✅ Czytelny |

### Kluczowe osiągnięcia refaktoryzacji

**Przed refaktoryzacją:**
```
❌ tasks.cpp       - inicjalizuje LVGL
❌ display.cpp     - inicjalizuje LVGL (duplikat!)
❌ ui_init.cpp     - inicjalizuje LVGL (duplikat!)
❌ 5+ plików       - hardkodowane piny GPIO
❌ touch_globals.cpp - globalne zmienne stanu
```

**Po refaktoryzacji:**
```
✅ boot.cpp        - JEDNA deterministyczna inicjalizacja
✅ defaults.h      - JEDNA centralna konfiguracja
✅ HAL classes     - enkapsulacja stanu, zero globali
✅ EventBus        - luźne wiązanie modułów
✅ Interfaces      - wymienialność implementacji
```
---

## 🎯 SEKCJA I: DODAWANIE KOMPONENTÓW UI

#### 1.2 KROK 1: Deklaracja ID ekranu

**Lokalizacja:** `src/ui/screens/screen_base.h`, linie ~17-27

**Zadanie:** Dodaj nowy identyfikator do enum `ScreenId`

```cpp
/**
 * @brief Screen identifiers
 * 
 * UWAGA: Kolejność ID powinna być logiczna (nie zmieniać istniejących!)
 * UWAGA: Zawsze dodawaj PRZED SCREEN_COUNT
 */
enum class ScreenId : uint8_t {
    SPLASH      = 0,    // Ekran powitalny (splash screen)
    HOME        = 1,    // Główny ekran aplikacji
    SETTINGS    = 2,    // Ekran ustawień
    DEMO        = 3,    // Ekran demo widgets LVGL
    
    // ═══════════════════════════════════════════════════════
    // DODAJ NOWE EKRANY TUTAJ (przed SCREEN_COUNT)
    // ═══════════════════════════════════════════════════════
    SENSOR_VIEW = 4,    // ← PRZYKŁAD: Ekran wizualizacji sensorów
    DATA_LOGGER = 5,    // ← PRZYKŁAD: Ekran rejestratora danych
    
    SCREEN_COUNT        // ← Automatyczny licznik (NIE USUWAĆ)
};
```

**Checklist:**
- [ ] ID dodane przed `SCREEN_COUNT`
- [ ] Komentarz opisujący cel ekranu
- [ ] Wartość numeryczna unikalna
- [ ] Nazwa zgodna z konwencją UPPER_SNAKE_CASE

---
┌─────────────────────────────────────────────────────┐
│              src/core/boot.cpp                       │
│         Unified Boot Sequence (8 etapów)            │
└─────────────────────────────────────────────────────┘
                        │
        ┌───────────────┼───────────────┐
        ▼               ▼               ▼
┌──────────────┐ ┌──────────────┐ ┌──────────────┐
│  HAL Layer   │ │  Services    │ │   UI Layer   │
│              │ │              │ │              │
│ IDisplay     │ │ EventBus     │ │ ScreenMgr    │
│ ITouch       │ │ Logger       │ │ ScreenBase   │
│ IStorage     │ │ (existing)   │ │ HomeScreen   │
└──────────────┘ └──────────────┘ └──────────────┘
```

#### 3. **Implementowane Komponenty**

| Komponent | Plik | Funkcja | Status |
|-----------|------|---------|--------|
| **Config** | `src/config/defaults.h` | Centralna konfiguracja (piny, stałe) | ✅ DONE |
| **HAL Interfaces** | `src/hal/I*.h` | Abstrakcja hardware | ✅ DONE |
| **Display Driver** | `src/hal/NV3041ADisplay.*` | Implementacja NV3041A | ✅ DONE |
| **Touch Driver** | `src/hal/GT911Touch.*` | Implementacja GT911 | ✅ DONE |
| **Unified Boot** | `src/core/boot.*` | Deterministyczny boot | ✅ DONE |
| **EventBus** | `src/services/event_bus.*` | Pub/Sub komunikacja | ✅ DONE |
| **Screen Manager** | `src/ui/screens/screen_manager.*` | Nawigacja + animacje | ✅ DONE |
| **Base Screen** | `src/ui/screens/screen_base.*` | Klasa bazowa ekranów | ✅ DONE |
| **Example Screen** | `src/ui/screens/home_screen.*` | Przykładowy ekran | ✅ DONE |
#### 1.3 KROK 2: Utworzenie plików nagłówkowego

**Lokalizacja:** `src/ui/screens/sensor_view_screen.h` (nowy plik)

**Szablon nagłówka:**

```cpp
/**
 * @file sensor_view_screen.h
 * @brief Ekran wizualizacji danych z sensorów
 * 
 * Ten ekran wyświetla w czasie rzeczywistym odczyty z:
 * - Czujnika temperatury (DS18B20)
 * - Czujnika wilgotności (DHT22)
 * - Barometru (BMP280)
 * 
 * @version 1.0.0
 * @date 2024-12-08
 */

#ifndef UI_SCREENS_SENSOR_VIEW_SCREEN_H
#define UI_SCREENS_SENSOR_VIEW_SCREEN_H

#include "screen_base.h"
#include "../../services/event_bus.h"

namespace UI {

/**
 * @brief Ekran wizualizacji danych sensorowych
 * 
 * Subskrybuje eventy:
 * - APP_TEMPERATURE_CHANGE
 * - APP_HUMIDITY_CHANGE
 * - APP_PRESSURE_CHANGE
 * 
 * Publikuje eventy:
 * - UI_BUTTON_CLICK (przycisk refresh)
#### 1.4 KROK 3: Implementacja pliku źródłowego

**Lokalizacja:** `src/ui/screens/sensor_view_screen.cpp` (nowy plik)

**Szablon implementacji z komentarzami:**

```cpp
/**
 * @file sensor_view_screen.cpp
 * @brief Implementacja ekranu wizualizacji sensorów
 */

#include "sensor_view_screen.h"
#include "screen_manager.h"
#include "../../config/defaults.h"
#include <Arduino.h>

namespace UI {

// ═══════════════════════════════════════════════════════════════
// KONSTRUKTOR / DESTRUKTOR
// ═══════════════════════════════════════════════════════════════

SensorViewScreen::SensorViewScreen()
    : ScreenBase(ScreenId::SENSOR_VIEW)  // ← Przekaż ID do klasy bazowej
    , _lblTemperature(nullptr)
    , _lblHumidity(nullptr)
    , _lblPressure(nullptr)
    , _btnRefresh(nullptr)
    , _chartTemp(nullptr)
    , _subTemp(0)
    , _subHumidity(0)
    , _subPressure(0)
{
    // UWAGA: NIE twórz tu widżetów LVGL!
    // Widżety tworzymy dopiero w create()
}

SensorViewScreen::~SensorViewScreen() {
    // UWAGA: Destruktor bazowy (ScreenBase) automatycznie
    // wywoła destroy() który usuwa wszystkie widżety
    
    // Jeśli masz własne zasoby (nie LVGL), zwolnij je tutaj
}

// ═══════════════════════════════════════════════════════════════
// WYMAGANE METODY Z ScreenBase
// ═══════════════════════════════════════════════════════════════

bool SensorViewScreen::create() {
    Serial.println("[SensorView] Creating screen...");
    
    // KROK 1: Utwórz bazowy kontener ekranu
    // ─────────────────────────────────────────────────────────
    if (!createScreenContainer()) {
        Serial.println("[SensorView] ERROR: Failed to create container!");
        return false;
    }
    
    // KROK 2: Dodaj title bar (opcjonalnie)
    // ─────────────────────────────────────────────────────────
    addTitleBar("Sensors");
    
    // KROK 3: Dodaj back button (opcjonalnie)
    // ─────────────────────────────────────────────────────────
    lv_obj_t* btnBack = addBackButton();
    lv_obj_add_event_cb(btnBack, [](lv_event_t* e) {
#### 1.5 KROK 4: Rejestracja ekranu w boot sequence

**Lokalizacja:** `src/core/boot.cpp`, funkcja `initUI()`, linie ~280-300

**Zadanie:** Zarejestruj ekran w ScreenManager

```cpp
#include "../ui/screens/sensor_view_screen.h"  // ← DODAJ include

static bool initUI(const BootConfig& config) {
    // ═══════════════════════════════════════════════════════
    // REJESTRACJA EKRANÓW
    // ═══════════════════════════════════════════════════════
    // UWAGA: Kolejność rejestracji nie ma znaczenia
    // UWAGA: ScreenManager przejmuje ownership (delete w destruktorze)
    
    SCREEN_MGR.registerScreen(new UI::HomeScreen());
    SCREEN_MGR.registerScreen(new UI::SensorViewScreen());  // ← DODAJ
    
    // Opcjonalnie: inne ekrany
    // SCREEN_MGR.registerScreen(new UI::SettingsScreen());
    // SCREEN_MGR.registerScreen(new UI::DataLoggerScreen());
    
    // ═══════════════════════════════════════════════════════
    // NAWIGACJA DO EKRANU STARTOWEGO
    // ═══════════════════════════════════════════════════════
    
    // Opcja A: Domyślnie HOME
    SCREEN_MGR.navigateTo(UI::ScreenId::HOME);
    
    // Opcja B: Twój nowy ekran jako startowy
    // SCREEN_MGR.navigateTo(UI::ScreenId::SENSOR_VIEW);
    
    // ═══════════════════════════════════════════════════════
    // DEMO LVGL (opcjonalnie)
    // ═══════════════════════════════════════════════════════
    
    if (config.startDemo) {
        lv_demo_widgets();
        Serial.println("[BOOT] Demo widgets started");
    }
    
    return true;
}
```

**Checklist rejestracji:**
- [ ] Include nagłówka dodany na górze `boot.cpp`
- [ ] `registerScreen(new ...)` wywołane w `initUI()`
- [ ] Domyślna nawigacja ustawiona
- [ ] Brak memory leaks (ScreenManager zwalnia automatycznie)

---

#### 1.6 KROK 5: Nawigacja do nowego ekranu

**Z dowolnego miejsca w kodzie:**

```cpp
/* Przykład 1: Z przycisku na HomeScreen */
static void onSensorsButtonClick(lv_event_t* e) {
    SCREEN_MGR.navigateTo(
        UI::ScreenId::SENSOR_VIEW,
        UI::ScreenTransition::SLIDE_LEFT,
        true  // pushHistory = true (umożliwia back button)
    );
}

/* Przykład 2: Z handlera EventBus */
EVENT_BUS.subscribe(
    Services::EventType::SYSTEM_ERROR,
    [](const Services::EventData& data) {
        // Przejdź do ekranu błędu
        SCREEN_MGR.navigateTo(
            UI::ScreenId::ERROR_SCREEN,
            UI::ScreenTransition::FADE
        );
    }
);

/* Przykład 3: Cofnięcie do poprzedniego ekranu */
void backButtonHandler(lv_event_t* e) {
    SCREEN_MGR.goBack(UI::ScreenTransition::SLIDE_RIGHT);
}
```

**Dostępne animacje przejść:**
```cpp
enum class ScreenTransition : uint8_t {
    NONE,           // Bez animacji (natychmiastowa zmiana)
    FADE,           // Płynne zanikanie
    SLIDE_LEFT,     // Przesunięcie w lewo
    SLIDE_RIGHT,    // Przesunięcie w prawo (typowo dla "back")
    SLIDE_UP,       // Przesunięcie w górę
    SLIDE_DOWN      // Przesunięcie w dół
};
```

---

#### 1.7 Kompletny checklist dodawania ekranu

**Przed commitem sprawdź:**

- [ ] **Deklaracja ID** w `screen_base.h`
- [ ] **Pliki .h i .cpp** utworzone w `src/ui/screens/`
- [ ] **Dziedziczenie** po `ScreenBase` poprawne
- [ ] **Konstruktor** inicjalizuje wszystkie zmienne członkowskie
- [ ] **Destruktor** zadeklarowany (nawet jeśli domyślny)
- [ ] **create()** - buduje UI, zwraca `bool`
- [ ] **onEnter()** - subskrybuje eventy, wywołuje `ScreenBase::onEnter()`
- [ ] **onExit()** - odsubskrybowuje eventy, wywołuje `ScreenBase::onExit()`
- [ ] **update()** - zaimplementowane (nawet jeśli puste)
- [ ] **getName()** - zwraca czytelną nazwę
- [ ] **Event handlers** są `static` z `user_data` cast
- [ ] **Include** dodany w `boot.cpp`
- [ ] **registerScreen()** wywołane w `initUI()`
- [ ] **Kompilacja** przechodzi bez błędów
- [ ] **Test nawigacji** - ekran pokazuje się poprawnie
- [ ] **Test back button** - powrót działa
- [ ] **Test eventów** - subskrypcje działają
- [ ] **Test memory** - brak leaks przy wielokrotnym wejściu/wyjściu

---d SensorViewScreen::onEnter() {
    // KROK 1: Wywołaj metodę bazową (WAŻNE!)
    // ─────────────────────────────────────────────────────────
    ScreenBase::onEnter();
    
    Serial.println("[SensorView] Entering screen...");
    
    // KROK 2: Subskrybuj eventy z EventBus
    // ─────────────────────────────────────────────────────────
    // UWAGA: Używamy lambda z capture [this] aby mieć dostęp do metod klasy
    
    _subTemp = EVENT_BUS.subscribe(
        Services::EventType::APP_TEMPERATURE_CHANGE,
        [this](const Services::EventData& data) {
            // Cast na konkretny typ eventu
            const auto& event = static_cast<const Services::FloatEvent&>(data);
            
            // Wywołaj metodę klasy
            updateTemperature(event.value);
            
            // Debug log
            Serial.printf("[SensorView] Temperature updated: %.1f°C\n", event.value);
        }
    );
    
    _subHumidity = EVENT_BUS.subscribe(
        Services::EventType::APP_CUSTOM_START + 1,  // Przykładowy custom event
        [this](const Services::EventData& data) {
            const auto& event = static_cast<const Services::FloatEvent&>(data);
            updateHumidity(event.value);
        }
    );
    
    _subPressure = EVENT_BUS.subscribe(
        Services::EventType::APP_CUSTOM_START + 2,
        [this](const Services::EventData& data) {
            const auto& event = static_cast<const Services::FloatEvent&>(data);
            
            if (_lblPressure) {
                lv_label_set_text_fmt(_lblPressure, "Pressure: %.0f hPa", event.value);
            }
        }
    );
    
    // KROK 3: Zażądaj odświeżenia danych (opcjonalnie)
    // ─────────────────────────────────────────────────────────
    // Publikuj event żeby moduł sensorów wysłał aktualne wartości
    EVENT_BUS.publish(Services::EventType::APP_CUSTOM_START + 10);  // REQUEST_SENSOR_DATA
    
    Serial.println("[SensorView] Subscriptions registered");
}

void SensorViewScreen::onExit() {
    Serial.println("[SensorView] Exiting screen...");
    
    // KROK 1: Odsubskrybuj WSZYSTKIE eventy (KRYTYCZNE!)
    // ─────────────────────────────────────────────────────────
    // UWAGA: Jeśli nie odsubskrybujesz, callbacki będą wywoływane
    //        nawet gdy ekran nieaktywny -> memory leak + crash!
    
    if (_subTemp != 0) {
        EVENT_BUS.unsubscribe(_subTemp);
        _subTemp = 0;
    }
    
    if (_subHumidity != 0) {
        EVENT_BUS.unsubscribe(_subHumidity);
        _subHumidity = 0;
    }
    
    if (_subPressure != 0) {
        EVENT_BUS.unsubscribe(_subPressure);
        _subPressure = 0;
    }
    
    // KROK 2: Wywołaj metodę bazową (WAŻNE!)
    // ─────────────────────────────────────────────────────────
    ScreenBase::onExit();
    
    Serial.println("[SensorView] Exited cleanly");
}

void SensorViewScreen::update() {
    // Ta metoda wywoływana jest CO KLATKĘ (~60 FPS) gdy ekran aktywny
    // 
    // UWAGI:
    // - NIE blokuj tu kodu (no delay, no long loops)
    // - Używaj tylko jeśli NAPRAWDĘ potrzebujesz (animacje, countdown)
    // - Dla aktualizacji danych UŻYJ EventBus zamiast polling tutaj
    
    // Przykład: animacja progress bar
    // static uint32_t lastUpdate = 0;
    // if (millis() - lastUpdate > 100) {  // Co 100ms
    //     lastUpdate = millis();
    //     // Twój kod animacji
    // }
}

// ═══════════════════════════════════════════════════════════════
// PRYWATNE METODY POMOCNICZE
// ═══════════════════════════════════════════════════════════════

void SensorViewScreen::createLayout() {
    // Przykładowy layout - 3 labele w kolumnie
    
    // Label temperatury
    _lblTemperature = lv_label_create(_screen);
    lv_obj_align(_lblTemperature, LV_ALIGN_CENTER, 0, -40);
    lv_obj_set_style_text_font(_lblTemperature, &lv_font_montserrat_14, 0);
    
    // Label wilgotności
    _lblHumidity = lv_label_create(_screen);
    lv_obj_align(_lblHumidity, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_text_font(_lblHumidity, &lv_font_montserrat_14, 0);
    
    // Label ciśnienia
    _lblPressure = lv_label_create(_screen);
    lv_obj_align(_lblPressure, LV_ALIGN_CENTER, 0, 40);
    lv_obj_set_style_text_font(_lblPressure, &lv_font_montserrat_14, 0);
    
    // Przycisk refresh
    _btnRefresh = lv_btn_create(_screen);
    lv_obj_set_size(_btnRefresh, 100, 40);
    lv_obj_align(_btnRefresh, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_add_event_cb(_btnRefresh, onRefreshClick, LV_EVENT_CLICKED, this);
    
    lv_obj_t* lblBtn = lv_label_create(_btnRefresh);
    lv_label_set_text(lblBtn, LV_SYMBOL_REFRESH " Refresh");
    lv_obj_center(lblBtn);
}

void SensorViewScreen::updateTemperature(float value) {
    if (_lblTemperature) {
        lv_label_set_text_fmt(_lblTemperature, "Temperature: %.1f°C", value);
        
        // Opcjonalnie: zmień kolor na podstawie wartości
        if (value > 30.0f) {
            lv_obj_set_style_text_color(_lblTemperature, lv_color_hex(0xFF0000), 0);  // Czerwony
        } else if (value < 15.0f) {
            lv_obj_set_style_text_color(_lblTemperature, lv_color_hex(0x0000FF), 0);  // Niebieski
        } else {
            lv_obj_set_style_text_color(_lblTemperature, lv_color_hex(0x00FF00), 0);  // Zielony
        }
    }
}

void SensorViewScreen::updateHumidity(float value) {
    if (_lblHumidity) {
        lv_label_set_text_fmt(_lblHumidity, "Humidity: %.0f%%", value);
    }
}

// ═══════════════════════════════════════════════════════════════
// EVENT HANDLERS (LVGL callbacks)
// ═══════════════════════════════════════════════════════════════

void SensorViewScreen::onRefreshClick(lv_event_t* e) {
    // KROK 1: Pobierz wskaźnik do this (przekazany jako user_data)
    // ─────────────────────────────────────────────────────────
    SensorViewScreen* screen = static_cast<SensorViewScreen*>(lv_event_get_user_data(e));
    
    // KROK 2: Zweryfikuj wskaźnik (defensive programming)
    // ─────────────────────────────────────────────────────────
    if (!screen) {
        Serial.println("[SensorView] ERROR: Invalid user_data in callback!");
        return;
    }
    
    // KROK 3: Wykonaj akcję
    // ─────────────────────────────────────────────────────────
    Serial.println("[SensorView] Refresh button clicked");
    
    // Publikuj event żądania odświeżenia
    EVENT_BUS.publish(Services::EventType::APP_CUSTOM_START + 10);
    
    // Opcjonalnie: wizualna informacja dla użytkownika
    lv_obj_t* lblBtn = lv_obj_get_child(screen->_btnRefresh, 0);
    if (lblBtn) {
        lv_label_set_text(lblBtn, LV_SYMBOL_REFRESH " Refreshing...");
        
        // Po 1 sekundzie przywróć oryginalny tekst (przykład)
        // W prawdziwej aplikacji użyj timer lub callback z EventBus
    }
}

} // namespace UI
```

**Checklist implementacji:**
- [ ] Include nagłówka ekranu
- [ ] Inicjalizacja wszystkich zmiennych w konstruktorze
- [ ] `create()` - buduje UI, zwraca bool
- [ ] `onEnter()` - subskrybuje eventy, wywołuje bazową
- [ ] `onExit()` - odsubskrybowuje WSZYSTKIE eventy, wywołuje bazową
- [ ] `update()` - zaimplementowane (nawet jeśli puste)
- [ ] Event handlers jako static z user_data cast
- [ ] Serial.println() do debugowania
- [ ] Namespace `UI`

--- Services::SubscriptionId _subHumidity;
    Services::SubscriptionId _subPressure;
    
    // ═══════════════════════════════════════════════════════
    // PRYWATNE METODY POMOCNICZE
    // ═══════════════════════════════════════════════════════
    
    /**
     * @brief Tworzy layout główny ekranu
     */
    void createLayout();
    
    /**
     * @brief Aktualizuje wartość temperatury na ekranie
     * @param value Temperatura w °C
     */
    void updateTemperature(float value);
    
    /**
     * @brief Aktualizuje wartość wilgotności na ekranie
     * @param value Wilgotność w %
     */
    void updateHumidity(float value);
    
    // ═══════════════════════════════════════════════════════
    // EVENT HANDLERS (LVGL callbacks - MUSZĄ BYĆ STATIC)
    // ═══════════════════════════════════════════════════════
    
    /**
     * @brief Handler kliknięcia przycisku Refresh
     * @param e Event LVGL
     * 
     * UWAGA: Metoda static - użyj lv_event_get_user_data() 
     *        aby dostać wskaźnik do this
     */
    static void onRefreshClick(lv_event_t* e);
};

} // namespace UI

#endif /* UI_SCREENS_SENSOR_VIEW_SCREEN_H */
```

**Checklist nagłówka:**
- [ ] Include guards (ifndef/define/endif)
- [ ] Doxygen comments dla klasy i metod publicznych
- [ ] Dziedziczenie po `ScreenBase`
- [ ] Konstruktor i destruktor zadeklarowane
- [ ] Wszystkie 5 wymaganych metod override
- [ ] Prywatne zmienne UI opisane komentarzami
- [ ] Event handlers jako `static`
- [ ] Namespace `UI`

---# Krok 2: Utwórz pliki ekranu

**Plik:** `src/ui/screens/moj_ekran.h`

```cpp
#ifndef UI_SCREENS_MOJ_EKRAN_H
#define UI_SCREENS_MOJ_EKRAN_H

#include "screen_base.h"

namespace UI {

class MojEkran : public ScreenBase {
public:
    MojEkran() : ScreenBase(ScreenId::NOWY_EKRAN) {}
    ~MojEkran() override = default;
    
    /* Implementacja wymaganych metod */
    bool create() override;
    void onEnter() override;
    void onExit() override;
    void update() override;
    const char* getName() const override { return "MojEkran"; }
    
private:
    /* Twoje zmienne UI */
    lv_obj_t* _mojButton;
    lv_obj_t* _mojLabel;
    
    /* Event handlery */
    static void onButtonClick(lv_event_t* e);
};

} // namespace UI

#endif
```

**Plik:** `src/ui/screens/moj_ekran.cpp`

```cpp
#include "moj_ekran.h"
#include "screen_manager.h"
#include "../../services/event_bus.h"

namespace UI {

bool MojEkran::create() {
    /* Utwórz kontener ekranu */
    if (!createScreenContainer()) {
        return false;
    }
    
    /* Dodaj title bar (opcjonalnie) */
    addTitleBar("Mój Ekran");
    
    /* Dodaj back button (opcjonalnie) */
    lv_obj_t* btnBack = addBackButton();
    lv_obj_add_event_cb(btnBack, [](lv_event_t* e) {
        SCREEN_MGR.goBack();
    }, LV_EVENT_CLICKED, nullptr);
    
    /* Utwórz swoje elementy UI */
    _mojButton = lv_btn_create(_screen);
    lv_obj_set_size(_mojButton, 120, 50);
    lv_obj_align(_mojButton, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_event_cb(_mojButton, onButtonClick, LV_EVENT_CLICKED, this);
    
    _mojLabel = lv_label_create(_mojButton);
    lv_label_set_text(_mojLabel, "Kliknij mnie!");
    lv_obj_center(_mojLabel);
    
    return true;
}

void MojEkran::onEnter() {
    ScreenBase::onEnter();
    Serial.println("[MojEkran] Wchodzę na ekran");
    
    /* Subskrybuj eventy jeśli potrzebujesz */
    // EVENT_BUS.subscribe(EventType::TOUCH_PRESSED, [](const EventData& e) { ... });
}

void MojEkran::onExit() {
    ScreenBase::onExit();
    /* Odsubskrybuj eventy */
}

void MojEkran::update() {
    /* Kod wykonywany co klatkę gdy ekran aktywny */
}

void MojEkran::onButtonClick(lv_event_t* e) {
    MojEkran* screen = (MojEkran*)lv_event_get_user_data(e);
    
    /* Publikuj event */
    EVENT_BUS.publish(Services::EventType::UI_BUTTON_CLICK);
    
    /* Zmień tekst */
    lv_label_set_text(screen->_mojLabel, "Kliknięto!");
}

} // namespace UI
```

#### Krok 3: Zarejestruj ekran w boot

**Plik:** `src/core/boot.cpp` (w funkcji `initUI`)

```cpp
static bool initUI(const BootConfig& config) {
    /* Zarejestruj ekrany */
    SCREEN_MGR.registerScreen(new UI::HomeScreen());
    SCREEN_MGR.registerScreen(new UI::MojEkran());  // ← Dodaj
    
    /* Nawiguj do home */
    SCREEN_MGR.navigateTo(UI::ScreenId::HOME);
    
    if (config.startDemo) {
        lv_demo_widgets();
    }
    return true;
}
```

#### Krok 4: Nawigacja między ekranami

Z dowolnego miejsca:

```cpp
/* Przejdź do nowego ekranu z animacją */
SCREEN_MGR.navigateTo(
    UI::ScreenId::NOWY_EKRAN,
    UI::ScreenTransition::SLIDE_LEFT
);

/* Cofnij się do poprzedniego */
SCREEN_MGR.goBack();
```

---

### 2️⃣ Jak używać EventBus (przepływ informacji)

#### Scenariusz: Powiadomienie o zmianie jasności

**Publisher (np. w SettingsScreen):**

```cpp
/* Zmień jasność */
getDisplay()->setBrightness(75);

/* Powiadom wszystkich subskrybentów */
EVENT_BUS.publishInt(
    Services::EventType::DISPLAY_BRIGHTNESS,
    75,
    0  // sourceId
);
```

**Subscriber (np. w StatusBar):**

```cpp
/* W onEnter() ekranu */
_subscriptionId = EVENT_BUS.subscribe(
    Services::EventType::DISPLAY_BRIGHTNESS,
    [this](const Services::EventData& data) {
        const auto& event = static_cast<const Services::IntEvent&>(data);
        
        /* Aktualizuj ikonkę jasności */
        lv_label_set_text_fmt(_lblBrightness, "%d%%", event.value);
    }
);

/* W onExit() ekranu */
EVENT_BUS.unsubscribe(_subscriptionId);
```

#### Dostępne typy eventów

Zobacz `src/services/event_bus.h`:

```cpp
enum class EventType : uint16_t {
    /* System */
    SYSTEM_BOOT_COMPLETE    = 0x0001,
    SYSTEM_LOW_MEMORY       = 0x0002,
    SYSTEM_ERROR            = 0x0003,
    
    /* Display */
    DISPLAY_READY           = 0x0100,
    DISPLAY_BRIGHTNESS      = 0x0101,
    DISPLAY_ROTATION        = 0x0102,
    
    /* Touch */
    TOUCH_PRESSED           = 0x0200,
    TOUCH_RELEASED          = 0x0201,
    
    /* Storage */
    STORAGE_MOUNTED         = 0x0300,
    STORAGE_ERROR           = 0x0302,
    
    /* UI */
    UI_SCREEN_CHANGE        = 0x0400,
    UI_BUTTON_CLICK         = 0x0401,
    
    /* Custom */
    APP_CUSTOM_START        = 0x1000,  // ← Twoje eventy tutaj
};
```

**Dodaj własny event:**

```cpp
enum class EventType : uint16_t {
    /* ... */
    APP_CUSTOM_START        = 0x1000,
    APP_TEMPERATURE_CHANGE  = 0x1001,  // ← Nowy
    APP_SENSOR_ERROR        = 0x1002,  // ← Nowy
};
```

---

### 3️⃣ Jak zmienić konfigurację hardware

**Wszystkie ustawienia w JEDNYM miejscu:** `src/config/defaults.h`

#### Zmiana pinu GPIO

```cpp
namespace DisplayConfig {
    constexpr uint8_t PIN_BL = 1;  // ← Zmień tutaj
}
```

Wszystkie komponenty automatycznie użyją nowej wartości.

#### Zmiana parametrów LVGL

```cpp
namespace LVGLConfig {
    constexpr uint8_t BUFFER_LINES = 40;     // ← Linie bufora
    constexpr bool USE_DOUBLE_BUFFER = false; // ← Podwójny bufor
}
```

#### Włączanie/wyłączanie funkcji

```cpp
namespace Features {
    constexpr bool ENABLE_SD_CARD       = true;
    constexpr bool ENABLE_LOGGING       = true;
    constexpr bool ENABLE_WIFI          = false;  // ← Przyszłość
    constexpr bool DEBUG_FPS_COUNTER    = true;   // ← Debug
}
```

---

### 4️⃣ Jak dodać nowy moduł hardware (HAL)

#### Przykład: Dodanie czujnika temperatury

**Krok 1: Utwórz interfejs**

**Plik:** `src/hal/ISensor.h`

```cpp
#ifndef HAL_ISENSOR_H
#define HAL_ISENSOR_H

namespace HAL {

class ISensor {
public:
    virtual ~ISensor() = default;
    
    virtual bool init() = 0;
    virtual void deinit() = 0;
    
    virtual float readValue() = 0;
    virtual bool isReady() const = 0;
};

} // namespace HAL

#endif
```

**Krok 2: Implementacja dla konkretnego czujnika**

**Plik:** `src/hal/DS18B20Sensor.h`

```cpp
#ifndef HAL_DS18B20_SENSOR_H
#define HAL_DS18B20_SENSOR_H

#include "ISensor.h"

namespace HAL {

class DS18B20Sensor : public ISensor {
public:
    DS18B20Sensor(uint8_t pin);
    ~DS18B20Sensor() override;
    
    bool init() override;
    void deinit() override;
    float readValue() override;
    bool isReady() const override;
    
private:
    uint8_t _pin;
    bool _initialized;
};

} // namespace HAL

#endif
```

**Krok 3: Dodaj do boot sequence**

**Plik:** `src/core/boot.cpp`

```cpp
#include "../hal/DS18B20Sensor.h"

static HAL::DS18B20Sensor* s_tempSensor = nullptr;

static bool initSensor(const BootConfig& config) {
    s_tempSensor = new HAL::DS18B20Sensor(15);  // GPIO 15
    
    if (!s_tempSensor->init()) {
        Serial.println("[BOOT] Sensor init failed!");
        return false;
    }
    
    Serial.println("[BOOT] Sensor initialized");
    return true;
}

/* W głównej funkcji boot() dodaj: */
s_currentStage = BootStage::SENSOR_INIT;
if (!initSensor(config)) {
    /* Nie fatal - kontynuuj */
}
```

**Krok 4: Użycie w aplikacji**

```cpp
/* Odczyt wartości */
float temp = getSensor()->readValue();

/* Publikuj event */
EVENT_BUS.publishFloat(
    Services::EventType::APP_TEMPERATURE_CHANGE,
    temp
);
```

---

### 5️⃣ Jak dodać task FreeRTOS

**Plik:** `src/core/tasks_new.cpp`

```cpp
/* Prototyp */
void mojaNowaTask(void* pvParameters);

/* Implementacja */
void mojaNowaTask(void* pvParameters) {
    Serial.println("[Task:Moja] Started");
    
    while (true) {
        /* Twój kod */
        float temp = getSensor()->readValue();
        
        /* Publikuj event */
        EVENT_BUS.publishFloat(
            Services::EventType::APP_TEMPERATURE_CHANGE,
            temp
        );
        
        /* Czekaj 1s */
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/* W createTasks() dodaj: */
bool createTasks() {
    /* ... istniejące taski ... */
    
    /* Nowy task */
    result = xTaskCreatePinnedToCore(
        mojaNowaTask,
        "MonitorTemp",
        4096,                           // Stack size
        nullptr,
        1,                              // Priority
        &s_mojTaskHandle,
        TaskConfig::SYSTEM_CORE         // Core 1
    );
    
    if (result != pdPASS) {
        Serial.println("[Tasks] Failed to create MonitorTemp task!");
        return false;
    }
    
    return true;
}
```

---

## 📐 ZASADY PROJEKTOWE

### ✅ DOs (Rób tak)

1. **Jedna odpowiedzialność** - każda klasa robi JEDNĄ rzecz
2. **Dependency Injection** - przekazuj zależności przez konstruktor
3. **EventBus do komunikacji** - nie wywołuj metod bezpośrednio między warstwami
4. **Dokumentuj publiczne API** - komentarze Doxygen nad każdą metodą
5. **Małe pliki** - max 200-300 linii na plik
6. **Namespace per warstwa** - `HAL::`, `UI::`, `Services::`
7. **RAII** - konstruktor inicjalizuje, destruktor zwalnia
8. **Const correctness** - `const` wszędzie gdzie to możliwe

### ❌ DON'Ts (Nie rób tego)

1. ❌ **Globalne zmienne** - użyj singletonów lub DI
2. ❌ **Hardkodowane wartości** - wszystko w `defaults.h`
3. ❌ **Direct hardware access w UI** - tylko przez HAL
4. ❌ **Długie funkcje** - max 50 linii, dziel na mniejsze
5. ❌ **God Objects** - klasa nie może wiedzieć o wszystkim
6. ❌ **Duplikacja kodu** - wydziel do funkcji pomocniczych
7. ❌ **Include cycles** - forward declarations + interfejsy

---

## 🔧 MAINTENANCE (Utrzymanie)

### Codzienne sprawdzenie

```bash
# Kompilacja
platformio run

# Sprawdź pamięć
# RAM:  [=         ]   9.8%
# Flash:[==        ]  19.3%

# Upload
platformio run --target upload

# Monitor
platformio device monitor --port COM8
```

### Przed commitem

```bash
# Usuń warnings
# Sprawdź czy nie ma memory leaks (heap usage stabilny)
# Dodaj komentarze do nowych funkcji publicznych
```

### Debugowanie

**Serial output:**
```cpp
Serial.printf("[MOJ_MODUL] Debug info: %d\n", value);
```

**Logger (do SD):**
```cpp
log_info("MOJ_MODUL", "Initialized successfully");
log_error("MOJ_MODUL", "Failed with error: %d", errorCode);
```

**EventBus stats:**
```cpp
uint32_t total = EVENT_BUS.getTotalEventsPublished();
uint8_t subs = EVENT_BUS.getSubscriberCount(EventType::UI_SCREEN_CHANGE);
Serial.printf("Events: %lu, Subscribers: %d\n", total, subs);
```

---

## 🚀 PRZEŁĄCZENIE NA NOWĄ ARCHITEKTURĘ

### Obecnie używana: STARA architektura

Kod działa w starej wersji (LvglWidgets.cpp).

### Aby przełączyć na NOWĄ:

**Metoda 1: Build Flag**

W `platformio.ini` odkomentuj:
```ini
build_flags =
    ...
    -DUSE_NEW_ARCHITECTURE  ; ← Usuń średnik
```

**Metoda 2: Rename**

```bash
mv src/LvglWidgets.cpp src/LvglWidgets.cpp.old
mv src/core/main_new.cpp src/main.cpp
```

Rebuild i upload:
```bash
platformio run --target upload
```

---

## 📚 STRUKTURA PLIKÓW - QUICK REFERENCE

```
src/
├── config/defaults.h              ← Zmień piny i stałe tutaj
│
├── core/
│   ├── boot.cpp                   ← Boot sequence, dodaj nowe moduły tutaj
│   ├── tasks_new.cpp              ← FreeRTOS tasks, dodaj nowe taski tutaj
│   └── main_new.cpp               ← Entry point (aktywowany flagą)
│
├── hal/
│   ├── IDisplay.h                 ← Interfejs display
│   ├── ITouch.h                   ← Interfejs touch
│   ├── NV3041ADisplay.cpp         ← Implementacja display
│   └── GT911Touch.cpp             ← Implementacja touch
│
├── services/
│   └── event_bus.cpp              ← Pub/Sub system, dodaj EventType tutaj
│
└── ui/screens/
    ├── screen_base.cpp            ← Bazowa klasa, NIE MODYFIKUJ
    ├── screen_manager.cpp         ← Manager ekranów, NIE MODYFIKUJ
    └── home_screen.cpp            ← Przykład - KOPIUJ jako szablon
```

---

## 📞 WSPARCIE

**Przy problemach sprawdź:**
1. `docs/MASTER_PLAN.md` - szczegółowa architektura
2. `docs/MIGRATION_GUIDE.md` - migracja z starego kodu
3. Komentarze w plikach `.h` - dokumentacja API

**Zasada:** Jeśli nie wiesz gdzie dodać kod, zapytaj się:
- Czy to logika hardware? → `hal/`
- Czy to komunikacja między modułami? → EventBus
- Czy to ekran UI? → `ui/screens/`
- Czy to konfiguracja? → `config/defaults.h`

---

**Powodzenia w rozbudowie! 🚀**
