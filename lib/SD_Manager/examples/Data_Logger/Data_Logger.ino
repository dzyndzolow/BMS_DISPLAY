/*
 * Przykład loggera danych
 * 
 * Ten przykład pokazuje jak używać karty SD jako rejestratora danych
 * zapisując odczyty z czujnika co określony czas
 */

#include <SD_Manager.h>

#define SD_CS    5
#define SPI_MOSI 23
#define SPI_MISO 19
#define SPI_SCK  18

SD_Manager sdCard(SD_CS, SPI_MOSI, SPI_MISO, SPI_SCK);

const char* logFile = "/sensor_log.csv";
unsigned long lastLogTime = 0;
const unsigned long logInterval = 5000; // Logowanie co 5 sekund
int logCounter = 0;

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n=== Data Logger Example ===\n");
    
    // Inicjalizacja karty SD
    if (!sdCard.begin()) {
        Serial.println("Błąd inicjalizacji karty SD!");
        while (1) delay(1000);
    }
    
    sdCard.printCardInfo();
    
    // Tworzenie nowego pliku logu z nagłówkiem
    Serial.println("Tworzenie pliku logu...");
    sdCard.writeFile(logFile, "Timestamp,Counter,Temperature,Humidity\n");
    
    Serial.println("Logger uruchomiony!");
    Serial.println("Dane będą zapisywane co 5 sekund...\n");
}

void loop() {
    unsigned long currentTime = millis();
    
    // Sprawdź czy minął czas do kolejnego zapisu
    if (currentTime - lastLogTime >= logInterval) {
        lastLogTime = currentTime;
        logCounter++;
        
        // Symulacja odczytów z czujników
        float temperature = 20.0 + random(-50, 50) / 10.0; // 15-25°C
        float humidity = 50.0 + random(-200, 200) / 10.0;  // 30-70%
        
        // Formatowanie danych
        char dataLine[100];
        sprintf(dataLine, "%lu,%d,%.1f,%.1f\n", 
                currentTime / 1000, logCounter, temperature, humidity);
        
        // Zapis do pliku
        if (sdCard.appendFile(logFile, dataLine)) {
            Serial.print("Zapisano: ");
            Serial.print(dataLine);
        } else {
            Serial.println("Błąd zapisu!");
        }
        
        // Co 10 odczytów wyświetl podsumowanie
        if (logCounter % 10 == 0) {
            Serial.println("\n--- Podsumowanie ---");
            Serial.print("Liczba zapisów: ");
            Serial.println(logCounter);
            
            size_t fileSize = sdCard.getFileSize(logFile);
            Serial.print("Rozmiar pliku: ");
            Serial.print(fileSize);
            Serial.println(" bajtów");
            
            uint64_t freeSpace = sdCard.getFreeSpace() / (1024 * 1024);
            Serial.print("Wolne miejsce: ");
            Serial.print((uint32_t)freeSpace);
            Serial.println(" MB\n");
        }
    }
    
    // Można tutaj dodać inne zadania
    delay(100);
}
