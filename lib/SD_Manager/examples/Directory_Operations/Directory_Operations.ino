/*
 * Przykład operacji na katalogach
 * 
 * Ten przykład pokazuje jak:
 * - Tworzyć katalogi
 * - Listować zawartość katalogów
 * - Usuwać katalogi
 * - Pracować ze strukturą katalogów
 */

#include <SD_Manager.h>

#define SD_CS    5
#define SPI_MOSI 23
#define SPI_MISO 19
#define SPI_SCK  18

SD_Manager sdCard(SD_CS, SPI_MOSI, SPI_MISO, SPI_SCK);

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n=== Directory Operations Example ===\n");
    
    // Inicjalizacja karty SD
    if (!sdCard.begin()) {
        Serial.println("Błąd inicjalizacji karty SD!");
        while (1) delay(1000);
    }
    
    // 1. Tworzenie katalogów
    Serial.println("1. Tworzenie struktury katalogów...");
    sdCard.createDir("/logs");
    sdCard.createDir("/data");
    sdCard.createDir("/config");
    
    // 2. Tworzenie plików w katalogach
    Serial.println("2. Tworzenie plików w katalogach...");
    sdCard.writeFile("/logs/log1.txt", "Log entry 1\n");
    sdCard.writeFile("/logs/log2.txt", "Log entry 2\n");
    sdCard.writeFile("/data/sensor1.csv", "time,value\n0,123\n1,456\n");
    sdCard.writeFile("/config/settings.txt", "param1=value1\nparam2=value2\n");
    
    // 3. Listowanie głównego katalogu
    Serial.println("\n3. Zawartość głównego katalogu:");
    sdCard.listDir("/", 0);
    
    // 4. Listowanie podkatalogu /logs
    Serial.println("\n4. Zawartość katalogu /logs:");
    sdCard.listDir("/logs", 0);
    
    // 5. Listowanie wszystkich katalogów z zagnieżdżeniem
    Serial.println("\n5. Pełna struktura katalogów (poziom 2):");
    sdCard.listDir("/", 2);
    
    // 6. Sprawdzanie czy katalog istnieje
    Serial.println("\n6. Sprawdzanie istnienia katalogów:");
    Serial.print("   /logs: ");
    Serial.println(sdCard.dirExists("/logs") ? "Istnieje" : "Nie istnieje");
    Serial.print("   /temp: ");
    Serial.println(sdCard.dirExists("/temp") ? "Istnieje" : "Nie istnieje");
    
    // 7. Czyszczenie - usuwanie plików i katalogów
    Serial.println("\n7. Czyszczenie testowych danych...");
    sdCard.deleteFile("/logs/log1.txt");
    sdCard.deleteFile("/logs/log2.txt");
    sdCard.removeDir("/logs");
    
    sdCard.deleteFile("/data/sensor1.csv");
    sdCard.removeDir("/data");
    
    sdCard.deleteFile("/config/settings.txt");
    sdCard.removeDir("/config");
    
    Serial.println("\nPrzykład zakończony!");
}

void loop() {
    delay(10000);
}
