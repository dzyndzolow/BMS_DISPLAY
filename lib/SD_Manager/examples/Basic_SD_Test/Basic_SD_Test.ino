/*
 * Przykład podstawowy - Test karty SD
 * 
 * Ten przykład pokazuje jak:
 * - Zainicjować kartę SD
 * - Wyświetlić informacje o karcie
 * - Przeprowadzić podstawowy test zapisu/odczytu
 */

#include <SD_Manager.h>

// Domyślne piny SPI dla ESP32
#define SD_CS    5
#define SPI_MOSI 23
#define SPI_MISO 19
#define SPI_SCK  18

// Utworzenie obiektu SD_Manager
SD_Manager sdCard(SD_CS, SPI_MOSI, SPI_MISO, SPI_SCK);

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n=== SD Card Basic Test ===\n");
    
    // Inicjalizacja karty SD
    if (!sdCard.begin()) {
        Serial.println("Nie można zainicjować karty SD!");
        Serial.println("Sprawdź:");
        Serial.println("1. Czy karta jest włożona");
        Serial.println("2. Czy podłączenie jest prawidłowe");
        Serial.println("3. Czy piny SPI są poprawne");
        while (1) delay(1000);
    }
    
    // Wyświetlenie informacji o karcie
    sdCard.printCardInfo();
    
    // Test funkcjonalności
    sdCard.testSDCard();
    
    Serial.println("Setup zakończony!");
}

void loop() {
    // Pusta pętla - test wykonuje się w setup()
    delay(10000);
}
