# Prompt dla LLM refaktoryzującego projekt (ESP32-S3 + LVGL)

Cel: uproszczenie i podział kodu użytkownika, tak aby główne pliki były krótkie (docelowo < ~1000 linii), czytelne, łatwe w utrzymaniu i skalowaniu. Biblioteki zewnętrzne pozostają bez zmian; reorganizujemy wyłącznie kod użytkownika.

## Wymagania ogólne
1) Zachowaj funkcjonalność 1:1 (brak regresji). Dodaj komentarze tylko tam, gdzie kod nie jest oczywisty.
2) Preferuj kompozycję i podział na moduły zamiast długich plików monolitów.
3) Nie twórz niepotrzebnych klas/warstw. Używaj prostych C/C++ modułów, gdy to czytelniejsze.
4) Każdy nowy plik ma jasno określoną odpowiedzialność.
5) Inicjalizacje w `setup`/głównej ścieżce pozostają, ale logika jest delegowana do modułów.
6) Unikaj duplikacji kodu; wyciągaj wspólne helpery.
7) Zachowaj istniejące piny/konfiguracje sprzętowe (patrz `SD_CONFIG.md`).

## Obecny kontekst
- Główny plik użytkownika: `src/LvglWidgets.cpp` (zbyt duży, mieszanka inicjalizacji, zadań, UI, logiki SD, loggera).
- Moduły już istniejące: `sd_card.cpp/h`, `logger.cpp/h`, UI w `lv_demo_widgets.c`.
- Środowisko: ESP32-S3, Arduino + LVGL, oddzielne piny SD i wyświetlacza.

## Docelowy podział (propozycja)
1) `src/app_main.cpp` (lub `main_app.cpp`):
   - Inicjalizacja głównych subsystemów: serial, gfx, touch, SD, logger, LVGL, uruchomienie tasków.
   - Rejestracja callbacków, start zadań FreeRTOS.
2) `src/ui/` (katalog):
   - `ui_init.cpp/h`: start LVGL, rejestracja display driver, input driver, uruchomienie demo/ekranów.
   - `ui_tabs_sd.cpp/h`: logika zakładki SD (tworzenie tabów, callbacks, przegląd plików, mount/format/test log), korzysta z interfejsu `sd_card` + `logger`.
   - `ui_tabs_settings.cpp/h`: zakładka Settings (brightness, reset, itp.), bez przycisku Rotate (usunięty).
   - (opcjonalnie) `ui_tabs_other.cpp/h`: pozostałe zakładki, jeśli są duże.
3) `src/system/` (opcjonalnie):
   - `display.cpp/h`: inicjalizacja i konfiguracja ekranu (gfx, backlight, pinout), jeśli da się wydzielić z głównego pliku.
   - `touch.cpp/h`: inicjalizacja dotyku (GT911).
   - `tasks.cpp/h`: tworzenie i definicje zadań FreeRTOS (np. `lvglTask`).
4) `src/sd/` (opcjonalnie):
   - już istniejący `sd_card.cpp/h` pozostaje; jeśli potrzeba, dodać helpery do listowania/browse.
5) `src/logging/` (opcjonalnie):
   - `logger.cpp/h` zostaje; ewentualne rozszerzenia (np. konfiguracja) w osobnym pliku.

## Zasady refaktoryzacji
- `LvglWidgets.cpp` ma stać się cienką warstwą: inicjalizacja + wywołanie funkcji z modułów. Przenieś długie funkcje i konfiguracje do dedykowanych plików.
- Każda zakładka LVGL → osobny plik, z własnymi statycznymi callbackami i ograniczonym zasięgiem (static). Eksportuj jedynie funkcję `tab_create(parent)` lub podobną.
- Konfiguracje sprzętowe/piny pozostają w jednym miejscu (np. `config_pins.h`), żeby uniknąć rozjazdów.
- Logika SD i loggera: korzystaj z istniejących modułów; w UI tylko wywołuj API (`sd_card_*`, `logger_*`).
- Display/touch init: jeżeli kod jest obszerny, wydziel do `display.cpp` / `touch.cpp` i eksportuj proste API `display_init()`, `touch_init()`.
- FreeRTOS taski: jeśli kod zadań jest długi, przenieś definicje do `tasks.cpp`, a w głównym pliku zostaw tylko `xTaskCreate` / `TaskHandle_t`.
- Maksymalna długość pojedynczego pliku użytkownika: dąż do < ~1000 linii; jeśli przekracza, rozbij na mniejsze moduły.
- Zachowaj istniejące ścieżki do zasobów i kolejność inicjalizacji (gfx -> display buf -> lvgl -> input -> UI -> tasks).

## Dobre praktyki (stosuj)
- Używaj `static` dla symboli o zasięgu pliku; eksportuj tylko potrzebne API.
- Nazwy plików i funkcji opisowe, spójne (np. `ui_sd_create_tab`, `ui_settings_create_tab`).
- Krótkie komentarze wyłącznie tam, gdzie intencja nie jest oczywista.
- Unikaj makr gdy wystarczą `constexpr`/`const`.
- Unikaj globali; gdy konieczne (LVGL wymaga), ogranicz ich liczbę i umieszczaj w dedykowanym module.
- Spójny styl: konsekwentne wcięcia, limit szerokości linii ~120.

## Wyjątki / rozsądek
- Nie rozbijaj kodu, jeśli rozbicie pogarsza czytelność (np. bardzo małe, spójne funkcje mogą pozostać razem).
- Jeśli zależności są silnie sprzężone (np. display driver setup + LVGL buf), możesz zostawić je w jednym module.
- Nie dotykaj plików bibliotek zewnętrznych (lvgl, Arduino_GFX, itp.).

## Plan działania dla LLM
1) Przejrzyj `LvglWidgets.cpp` i zidentyfikuj bloki do wydzielenia (UI zakładki, display/touch init, taski).
2) Zaproponuj nową strukturę plików (wg powyższych katalogów), następnie utwórz/zmodyfikuj pliki.
3) Przenieś kod: najpierw wytnij logikę zakładek do `ui_tabs_*.cpp/h`, potem display/touch do swoich modułów, na końcu uporządkuj główny plik.
4) Upewnij się, że `setup`/główne entrypointy wywołują nowe API w poprawnej kolejności.
5) Zbuduj projekt i zweryfikuj brak regresji (jeśli środowisko pozwala, uruchom upload/testy).

## Krótka instrukcja dla modelu (one-shot)
"Przenieś długi kod z `src/LvglWidgets.cpp` do mniejszych modułów: UI zakładki do `src/ui/ui_tabs_*.cpp/h`, inicjalizacja display/touch do `src/system/display.cpp` i `touch.cpp` (jeśli ma sens), zadania FreeRTOS do `src/system/tasks.cpp`. Utrzymaj piny/konfigurację, nie zmieniaj bibliotek. Główne pliki mają być krótsze (<~1000 linii), czytelne, bez utraty funkcji."
