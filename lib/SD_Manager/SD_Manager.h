#ifndef SD_MANAGER_H
#define SD_MANAGER_H

#include "Arduino.h"
#include "FS.h"
#include "SD.h"
#include "SPI.h"

class SD_Manager {
public:
    // Konstruktor z pinami dla JC4827W543 board (złącze TF)
    SD_Manager(uint8_t cs_pin = 10, uint8_t mosi = 11, uint8_t miso = 13, uint8_t sck = 12);
    
    // Inicjalizacja karty SD
    bool begin(uint32_t frequency = 4000000);
    
    // Zakończenie pracy z kartą SD
    void end();
    
    // Informacje o karcie SD
    uint64_t getCardSize();           // Rozmiar karty w bajtach
    uint64_t getUsedSpace();          // Użyta przestrzeń w bajtach
    uint64_t getFreeSpace();          // Wolna przestrzeń w bajtach
    uint8_t getCardType();            // Typ karty (CARD_NONE, CARD_MMC, CARD_SD, CARD_SDHC)
    String getCardTypeString();       // Typ karty jako tekst
    
    // Operacje na plikach
    bool fileExists(const char* path);
    bool createFile(const char* path);
    bool deleteFile(const char* path);
    bool renameFile(const char* oldPath, const char* newPath);
    size_t getFileSize(const char* path);
    
    // Odczyt plików
    String readFile(const char* path);
    bool readFile(const char* path, uint8_t* buffer, size_t len);
    
    // Zapis do plików
    bool writeFile(const char* path, const char* message);
    bool writeFile(const char* path, const uint8_t* data, size_t len);
    bool appendFile(const char* path, const char* message);
    
    // Operacje na katalogach
    bool createDir(const char* path);
    bool removeDir(const char* path);
    bool dirExists(const char* path);
    void listDir(const char* dirname, uint8_t levels = 0);
    
    // Test karty SD
    bool testSDCard();
    void printCardInfo();
    
    // Sprawdzenie czy karta jest zamontowana
    bool isCardMounted();

private:
    uint8_t _cs_pin;
    uint8_t _mosi_pin;
    uint8_t _miso_pin;
    uint8_t _sck_pin;
    bool _initialized;
    SPIClass* _spi;  // Dedykowana instancja SPI dla karty SD
    
    void listDirRecursive(File dir, uint8_t numTabs);
};

#endif // SD_MANAGER_H
