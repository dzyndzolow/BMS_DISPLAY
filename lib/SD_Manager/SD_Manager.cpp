#include "SD_Manager.h"

SD_Manager::SD_Manager(uint8_t cs_pin, uint8_t mosi, uint8_t miso, uint8_t sck) {
    _cs_pin = cs_pin;
    _mosi_pin = mosi;
    _miso_pin = miso;
    _sck_pin = sck;
    _initialized = false;
    _spi = new SPIClass(HSPI);  // Dedykowana magistrala HSPI dla SD
}

bool SD_Manager::begin(uint32_t frequency) {
    pinMode(_cs_pin, OUTPUT);
    digitalWrite(_cs_pin, HIGH);
    
    // Inicjalizacja dedykowanej magistrali HSPI
    _spi->begin(_sck_pin, _miso_pin, _mosi_pin, _cs_pin);
    _spi->setFrequency(frequency);
    
    // Montowanie karty SD z dedykowaną magistralą SPI
    _initialized = SD.begin(_cs_pin, *_spi, frequency);
    
    if (_initialized) {
        Serial.println("SD Card initialized successfully");
        printCardInfo();
    } else {
        Serial.println("SD Card initialization failed!");
        Serial.println("Please check: 1) Card is FAT32 formatted, 2) Wiring is correct");
    }
void SD_Manager::end() {
    SD.end();
    _spi->end();
    _initialized = false;
}
void SD_Manager::end() {
    SD.end();
    _initialized = false;
}

uint64_t SD_Manager::getCardSize() {
    if (!_initialized) return 0;
    return SD.cardSize();
}

uint64_t SD_Manager::getUsedSpace() {
    if (!_initialized) return 0;
    return SD.usedBytes();
}

uint64_t SD_Manager::getFreeSpace() {
    if (!_initialized) return 0;
    return getCardSize() - getUsedSpace();
}

uint8_t SD_Manager::getCardType() {
    if (!_initialized) return CARD_NONE;
    return SD.cardType();
}

String SD_Manager::getCardTypeString() {
    uint8_t cardType = getCardType();
    
    switch(cardType) {
        case CARD_NONE:
            return "No SD card";
        case CARD_MMC:
            return "MMC";
        case CARD_SD:
            return "SDSC";
        case CARD_SDHC:
            return "SDHC";
        default:
            return "Unknown";
    }
}

bool SD_Manager::isCardMounted() {
    return _initialized && (getCardType() != CARD_NONE);
}

bool SD_Manager::fileExists(const char* path) {
    if (!_initialized) return false;
    return SD.exists(path);
}

bool SD_Manager::createFile(const char* path) {
    if (!_initialized) return false;
    
    File file = SD.open(path, FILE_WRITE);
    if (!file) {
        Serial.println("Failed to create file");
        return false;
    }
    file.close();
    return true;
}

bool SD_Manager::deleteFile(const char* path) {
    if (!_initialized) return false;
    
    if (!SD.exists(path)) {
        Serial.println("File doesn't exist");
        return false;
    }
    
    if (SD.remove(path)) {
        Serial.println("File deleted");
        return true;
    } else {
        Serial.println("Delete failed");
        return false;
    }
}

bool SD_Manager::renameFile(const char* oldPath, const char* newPath) {
    if (!_initialized) return false;
    
    if (!SD.exists(oldPath)) {
        Serial.println("Source file doesn't exist");
        return false;
    }
    
    if (SD.rename(oldPath, newPath)) {
        Serial.println("File renamed");
        return true;
    } else {
        Serial.println("Rename failed");
        return false;
    }
}

size_t SD_Manager::getFileSize(const char* path) {
    if (!_initialized) return 0;
    
    File file = SD.open(path);
    if (!file) return 0;
    
    size_t size = file.size();
    file.close();
    return size;
}

String SD_Manager::readFile(const char* path) {
    if (!_initialized) return "";
    
    File file = SD.open(path);
    if (!file) {
        Serial.println("Failed to open file for reading");
        return "";
    }
    
    String content = "";
    while (file.available()) {
        content += (char)file.read();
    }
    
    file.close();
    return content;
}

bool SD_Manager::readFile(const char* path, uint8_t* buffer, size_t len) {
    if (!_initialized) return false;
    
    File file = SD.open(path);
    if (!file) {
        Serial.println("Failed to open file for reading");
        return false;
    }
    
    size_t bytesRead = file.read(buffer, len);
    file.close();
    
    return (bytesRead == len);
}

bool SD_Manager::writeFile(const char* path, const char* message) {
    if (!_initialized) return false;
    
    File file = SD.open(path, FILE_WRITE);
    if (!file) {
        Serial.println("Failed to open file for writing");
        return false;
    }
    
    bool success = file.print(message);
    file.close();
    
    if (success) {
        Serial.println("File written");
    } else {
        Serial.println("Write failed");
    }
    
    return success;
}

bool SD_Manager::writeFile(const char* path, const uint8_t* data, size_t len) {
    if (!_initialized) return false;
    
    File file = SD.open(path, FILE_WRITE);
    if (!file) {
        Serial.println("Failed to open file for writing");
        return false;
    }
    
    size_t written = file.write(data, len);
    file.close();
    
    return (written == len);
}

bool SD_Manager::appendFile(const char* path, const char* message) {
    if (!_initialized) return false;
    
    File file = SD.open(path, FILE_APPEND);
    if (!file) {
        Serial.println("Failed to open file for appending");
        return false;
    }
    
    bool success = file.print(message);
    file.close();
    
    if (success) {
        Serial.println("Message appended");
    } else {
        Serial.println("Append failed");
    }
    
    return success;
}

bool SD_Manager::createDir(const char* path) {
    if (!_initialized) return false;
    
    if (SD.mkdir(path)) {
        Serial.println("Directory created");
        return true;
    } else {
        Serial.println("mkdir failed");
        return false;
    }
}

bool SD_Manager::removeDir(const char* path) {
    if (!_initialized) return false;
    
    if (SD.rmdir(path)) {
        Serial.println("Directory removed");
        return true;
    } else {
        Serial.println("rmdir failed");
        return false;
    }
}

bool SD_Manager::dirExists(const char* path) {
    if (!_initialized) return false;
    
    File dir = SD.open(path);
    if (!dir) return false;
    
    bool isDir = dir.isDirectory();
    dir.close();
    return isDir;
}

void SD_Manager::listDir(const char* dirname, uint8_t levels) {
    if (!_initialized) {
        Serial.println("SD Card not initialized");
        return;
    }
    
    Serial.printf("Listing directory: %s\n", dirname);
    
    File root = SD.open(dirname);
    if (!root) {
        Serial.println("Failed to open directory");
        return;
    }
    
    if (!root.isDirectory()) {
        Serial.println("Not a directory");
        return;
    }
    
    listDirRecursive(root, levels);
    root.close();
}

void SD_Manager::listDirRecursive(File dir, uint8_t numTabs) {
    while (true) {
        File entry = dir.openNextFile();
        if (!entry) {
            break;
        }
        
        for (uint8_t i = 0; i < numTabs; i++) {
            Serial.print('\t');
        }
        
        Serial.print(entry.name());
        
        if (entry.isDirectory()) {
            Serial.println("/");
            listDirRecursive(entry, numTabs + 1);
        } else {
            Serial.print("\t\t");
            Serial.println(entry.size());
        }
        
        entry.close();
    }
}

bool SD_Manager::testSDCard() {
    if (!_initialized) {
        Serial.println("SD Card not initialized");
        return false;
    }
    
    Serial.println("\n=== SD Card Test ===");
    
    // Test zapisu
    const char* testFile = "/test.txt";
    const char* testData = "Hello SD Card!";
    
    Serial.println("Writing test file...");
    if (!writeFile(testFile, testData)) {
        return false;
    }
    
    // Test odczytu
    Serial.println("Reading test file...");
    String content = readFile(testFile);
    Serial.print("Read content: ");
    Serial.println(content);
    
    if (content != testData) {
        Serial.println("Content mismatch!");
        return false;
    }
    
    // Test dopisywania
    Serial.println("Appending to file...");
    appendFile(testFile, "\nAppended text");
    
    // Test usuwania
    Serial.println("Deleting test file...");
    if (!deleteFile(testFile)) {
        return false;
    }
    
    Serial.println("=== Test Passed ===\n");
    return true;
}

void SD_Manager::printCardInfo() {
    if (!_initialized) {
        Serial.println("SD Card not initialized");
        return;
    }
    
    Serial.println("\n=== SD Card Info ===");
    Serial.print("Card Type: ");
    Serial.println(getCardTypeString());
    
    uint64_t cardSize = getCardSize() / (1024 * 1024);
    Serial.print("Card Size: ");
    Serial.print(cardSize);
    Serial.println(" MB");
    
    uint64_t usedSize = getUsedSpace() / (1024 * 1024);
    Serial.print("Used Space: ");
    Serial.print(usedSize);
    Serial.println(" MB");
    
    uint64_t freeSize = getFreeSpace() / (1024 * 1024);
    Serial.print("Free Space: ");
    Serial.print(freeSize);
    Serial.println(" MB");
    
    Serial.println("====================\n");
}
