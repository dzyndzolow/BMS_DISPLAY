# ESP32-S3 Web Interface

Profesjonalny interfejs webowy dla urządzenia ESP32-S3 z wyświetlaczem LVGL.

## 📁 Struktura folderów

```
www/
├── index.html              # Strona główna - panel sterowania
├── css/
│   └── simple.min.css      # Simple.css framework (10KB)
├── js/
│   └── app.js              # Logika aplikacji, API calls
├── pages/
│   └── diagnostics.html    # Strona diagnostyczna
└── assets/                 # Obrazy, ikony (opcjonalnie)
```

## 🎨 Funkcje

### Strona główna (`index.html`)
- 📊 Status systemu w czasie rzeczywistym
  - Czas działania (uptime)
  - Zużycie pamięci RAM
  - Siła sygnału WiFi (RSSI)
  - Temperatura
- 🎛️ Sterowanie wyświetlaczem
  - Slider jasności (0-100%)
  - Włącz/Wyłącz ekran
- ⚡ Szybkie akcje
  - Restart urządzenia
  - Czyszczenie logów
- 🔄 Auto-refresh co 5 sekund

### Strona diagnostyczna (`pages/diagnostics.html`)
- 💻 Szczegóły sprzętu
  - Model chipu, rdzenie, częstotliwość
  - Pamięć RAM, PSRAM, Flash
  - Informacje o wyświetlaczu i touch
- 📡 Diagnostyka sieci
  - Status WiFi, SSID, IP, MAC
  - RSSI, status serwera web
  - Statystyki SD Card
- 📝 Logi systemowe
  - Podgląd logów w czasie rzeczywistym
  - Eksport logów do pliku
- ⚙️ Testy diagnostyczne
  - Test wyświetlacza
  - Test dotyku
  - Test karty SD
  - Test połączenia (ping)

## 🌐 API Endpoints

Interfejs komunikuje się z ESP32 przez REST API:

```
GET  /api/status        - Status systemu
POST /api/brightness    - Ustawienie jasności {brightness: 0-100}
POST /api/screen        - Sterowanie ekranem {action: "on"|"off"}
POST /api/restart       - Restart urządzenia
GET  /api/logs          - Pobierz logi
DELETE /api/logs        - Wyczyść logi
GET  /api/diagnostics   - Szczegóły diagnostyczne
```

## 🎨 Stylowanie

Projekt używa **Simple.css** framework:
- ✅ Automatyczne dark/light mode
- ✅ Responsywny design
- ✅ Tylko ~10KB (minified)
- ✅ Zero klas CSS (semantic HTML)
- ✅ Piękne domyślne style

## 📦 Wdrożenie na ESP32

### Krok 1: Skopiuj pliki na kartę SD

Użyj skryptu PowerShell:
```powershell
.\copy_from_d_drive.ps1
```

Lub ręcznie:
```
Skopiuj zawartość D:\www\ na kartę SD do folderu www/
```

### Krok 2: Włóż kartę SD do ESP32

Struktura na karcie SD:
```
E:\
└── www\
    ├── index.html
    ├── css\
    │   └── simple.min.css
    ├── js\
    │   └── app.js
    └── pages\
        └── diagnostics.html
```

### Krok 3: Uruchom urządzenie

1. Połącz ESP32 z WiFi
2. Otwórz przeglądarkę
3. Wpisz adres IP urządzenia (np. `http://192.168.1.100`)
4. Gotowe! 🎉

## 🛠️ Rozwój lokalny

### Test w przeglądarce (bez ESP32)

```powershell
# Otwórz index.html bezpośrednio
start www\index.html
```

**Uwaga:** API calls będą się nie powodzić (brak ESP32), ale możesz sprawdzić layout i UI.

### Edycja plików

Pliki znajdują się w:
- `c:\JC4827W543_Integrated\www\` (junction do D:\www)

Edytuj dowolny plik w VS Code - zmiany zapisują się bezpośrednio w `D:\www`.

## 🔧 Konfiguracja

### Zmiana czasu auto-refresh

Edytuj `js/app.js`:
```javascript
const REFRESH_INTERVAL = 5000; // 5 sekund (5000ms)
```

### Zmiana koloru accent

Dodaj custom.css:
```css
:root {
  --accent: #ff6b6b; /* Czerwony zamiast niebieskiego */
}
```

## 📱 Kompatybilność

✅ Chrome/Edge (Desktop & Mobile)  
✅ Firefox (Desktop & Mobile)  
✅ Safari (iOS)  
✅ Opera  
⚠️ IE11 (nie wspierany)

## 📄 Licencja

Simple.css: MIT License  
Projekt ESP32: Custom

## 🤝 Wsparcie

W razie problemów:
1. Sprawdź logi ESP32 przez Serial Monitor
2. Sprawdź Console w przeglądarce (F12)
3. Zweryfikuj czy karta SD jest prawidłowo zamontowana
