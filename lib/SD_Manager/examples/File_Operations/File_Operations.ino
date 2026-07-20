/*
 * Przykład operacji na plikach
 * 
 * Ten przykład pokazuje jak:
 * - Tworzyć pliki
 * - Zapisywać i odczytywać dane
 * - Dopisywać do istniejących plików
 * - Zmieniać nazwy i usuwać pliki
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
    
    Serial.println("\n=== File Operations Example ===\n");
    
    // Inicjalizacja karty SD
    if (!sdCard.begin()) {
        Serial.println("Błąd inicjalizacji karty SD!");
        while (1) delay(1000);
    }
    
    // 1. Tworzenie i zapis do pliku
    Serial.println("1. Tworzenie pliku 'data.txt'...");
    sdCard.writeFile("/data.txt", "Początkowa linia tekstu\n");
    
    // 2. Odczyt zawartości pliku
    Serial.println("2. Odczyt pliku:");
    String content = sdCard.readFile("/data.txt");
    Serial.print(content);
    
    // 3. Dopisanie do pliku
    Serial.println("3. Dopisywanie do pliku...");
    sdCard.appendFile("/data.txt", "Druga linia tekstu\n");
    sdCard.appendFile("/data.txt", "Trzecia linia tekstu\n");
    
    // 4. Odczyt zaktualizowanego pliku
    Serial.println("4. Odczyt zaktualizowanego pliku:");
    content = sdCard.readFile("/data.txt");
    Serial.print(content);
    
    // 5. Sprawdzenie rozmiaru pliku
    size_t fileSize = sdCard.getFileSize("/data.txt");
    Serial.print("5. Rozmiar pliku: ");
    Serial.print(fileSize);
    Serial.println(" bajtów");
    
    // 6. Zmiana nazwy pliku
    Serial.println("6. Zmiana nazwy pliku na 'backup.txt'...");
    sdCard.renameFile("/data.txt", "/backup.txt");
    
    // 7. Sprawdzenie czy plik istnieje
    Serial.print("7. Czy 'data.txt' istnieje? ");
    Serial.println(sdCard.fileExists("/data.txt") ? "Tak" : "Nie");
    Serial.print("   Czy 'backup.txt' istnieje? ");
    Serial.println(sdCard.fileExists("/backup.txt") ? "Tak" : "Nie");
    
    // 8. Usunięcie pliku
    Serial.println("8. Usuwanie pliku 'backup.txt'...");
    sdCard.deleteFile("/backup.txt");
    
    Serial.println("\nPrzykład zakończony!");
}

void loop() {
    delay(10000);
}
