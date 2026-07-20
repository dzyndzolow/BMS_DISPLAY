# SD_Manager - Biblioteka do obsługi kart SD dla ESP32

Kompletna i łatwa w użyciu biblioteka do zarządzania kartami SD na ESP32 przez interfejs SPI.

## Funkcje

- ✅ Łatwa inicjalizacja karty SD z dedykowaną magistralą HSPI
- ✅ Pełna obsługa plików (tworzenie, odczyt, zapis, usuwanie)
- ✅ Operacje na katalogach
- ✅ Informacje o karcie (typ, rozmiar, wolne miejsce)
- ✅ Funkcje testowe
- ✅ Szczegółowe komunikaty błędów
- ✅ Bezkonfliktowa praca z wyświetlaczami QSPI/SPI
- ✅ Optymalizacja dla audio playback (1MHz SPI)

## Instalacja

1. Skopiuj folder `SD_Manager` do folderu `libraries` w Arduino
2. Zrestartuj Arduino IDE
3. Biblioteka będzie dostępna w `Sketch > Include Library > SD_Manager`

## Wymagania

- ESP32
- Karta microSD
- Moduł czytnika SD (SPI)

## Domyślne piny SPI dla JC4827W543 board

```cpp
#define SD_CS    10   // Chip Select (TF_CS)
#define SPI_MOSI 11   // Master Out Slave In (TF_MOSI)
#define SPI_MISO 13   // Master In Slave Out (TF_MISO)
#define SPI_SCK  12   // Clock (TF_CLK)
```

## Podstawowe użycie

```cpp
#include <SD_Manager.h>

// Utworzenie obiektu z domyślnymi pinami dla JC4827W543
SD_Manager sdCard(10, 11, 13, 12);

void setup() {
    Serial.begin(115200);
    
    // Inicjalizacja karty SD z częstotliwością 4MHz
    if (!sdCard.begin(4000000)) {
        Serial.println("Błąd inicjalizacji karty SD!");
        return;
    }
    
    // Wyświetl informacje o karcie
    sdCard.printCardInfo();
    
    // Zapis do pliku
    sdCard.writeFile("/test.txt", "Hello SD Card!");
    
    // Odczyt z pliku
    String content = sdCard.readFile("/test.txt");
    Serial.println(content);
}

void loop() {
    // Twój kod
}
```

## Przykłady

Biblioteka zawiera cztery kompletne przykłady:

1. **Basic_SD_Test** - Podstawowy test karty SD
2. **File_Operations** - Operacje na plikach
3. **Directory_Operations** - Zarządzanie katalogami
4. **Data_Logger** - Logger danych z czujników

## API

### Inicjalizacja

```cpp
SD_Manager(uint8_t cs_pin, uint8_t mosi, uint8_t miso, uint8_t sck)
bool begin(uint32_t frequency = 1000000)
void end()
```

### Informacje o karcie

```cpp
uint64_t getCardSize()        // Rozmiar w bajtach
uint64_t getUsedSpace()       // Użyta przestrzeń
uint64_t getFreeSpace()       // Wolna przestrzeń
String getCardTypeString()    // Typ karty (SDSC, SDHC, etc.)
bool isCardMounted()          // Czy karta jest zamontowana
void printCardInfo()          // Wyświetl szczegółowe informacje
```

### Operacje na plikach

```cpp
bool fileExists(const char* path)
bool createFile(const char* path)
bool deleteFile(const char* path)
bool renameFile(const char* oldPath, const char* newPath)
size_t getFileSize(const char* path)
String readFile(const char* path)
bool writeFile(const char* path, const char* message)
bool appendFile(const char* path, const char* message)
```

### Operacje na katalogach

```cpp
bool createDir(const char* path)
bool removeDir(const char* path)
bool dirExists(const char* path)
void listDir(const char* dirname, uint8_t levels = 0)
```

### Testy

```cpp
bool testSDCard()  // Kompleksowy test funkcjonalności
```

## Licencja

Ta biblioteka jest darmowa i może być używana w projektach prywatnych i komercyjnych.

## Autor

Stworzone na podstawie przykładów ESP32-audioI2S i oficjalnej biblioteki SD.

## Wersja

1.0.0 - Pierwsza wersja (Grudzień 2025)
