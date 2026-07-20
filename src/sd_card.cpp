/*SD Card implementation for ESP32-S3*/
#include "sd_card.h"
#include <Arduino.h>
#include <SD.h>
#include <SPI.h>

/* SD Card pins for JC4827W543 board
 * According to board schematic:
 * TF_CS   -> IO10
 * TF_MOSI -> IO11 (Data IN to SD card)
 * TF_CLK  -> IO12
 * TF_MISO -> IO13 (Data OUT from SD card)
 */
#define SD_CS_PIN   10  /*TF_CS*/
#define SD_MOSI_PIN 11  /*TF_MOSI - MCU to SD card*/
#define SD_SCK_PIN  12  /*TF_CLK*/
#define SD_MISO_PIN 13  /*TF_MISO - SD card to MCU*/

/*Dedicated SPI instance for SD card to avoid conflicts with display*/
static SPIClass sdSPI(HSPI);

static bool sd_initialized = false;

bool sd_card_init(void) {
    if(sd_initialized) {
        return true;
    }

    Serial.println("SD Card init starting...");
    Serial.printf("  CS=%d, MOSI=%d, SCK=%d, MISO=%d\n", SD_CS_PIN, SD_MOSI_PIN, SD_SCK_PIN, SD_MISO_PIN);

    /*Initialize dedicated SPI for SD card*/
    sdSPI.begin(SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);

    /*Mount SD card using dedicated SPI bus*/
    if(!SD.begin(SD_CS_PIN, sdSPI, 4000000)) {  /*4MHz*/
        Serial.println("SD Card Mount Failed!");
        Serial.println("Please format card as FAT32 on computer");
        return false;
    }

    uint8_t card_type = SD.cardType();
    if(card_type == CARD_NONE) {
        Serial.println("No SD card attached");
        return false;
    }

    Serial.print("SD Card Type: ");
    if(card_type == CARD_MMC) {
        Serial.println("MMC");
    } else if(card_type == CARD_SD) {
        Serial.println("SDSC");
    } else if(card_type == CARD_SDHC) {
        Serial.println("SDHC");
    } else {
        Serial.println("UNKNOWN");
    }

    uint64_t card_size = SD.cardSize() / (1024 * 1024);
    Serial.printf("SD Card Size: %llu MB\n", card_size);

    sd_initialized = true;
    Serial.println("SD Card initialized successfully");
    return true;
}

bool sd_card_is_mounted(void) {
    return sd_initialized;
}

bool sd_card_unmount(void) {
    if(!sd_initialized) return true;
    SD.end();
    sd_initialized = false;
    return true;
}

void sd_card_list_dir(const char * path) {
    if(!sd_initialized) {
        Serial.println("SD Card not initialized");
        return;
    }

    File root = SD.open(path);
    if(!root || !root.isDirectory()) {
        Serial.println("Failed to open directory");
        return;
    }
    File file = root.openNextFile();
    while(file) {
        if(file.isDirectory()) {
            Serial.print("DIR : ");
            Serial.println(file.name());
        } else {
            Serial.print("FILE: ");
            Serial.print(file.name());
            Serial.print(" SIZE: ");
            Serial.println(file.size());
        }
        file = root.openNextFile();
    }
}

int sd_card_list_dir_iter(const char * path, void (*cb)(const char * name, bool is_dir, uint32_t size, void * ctx), void * ctx) {
    if(!sd_initialized) return -1;

    File root = SD.open(path);
    if(!root || !root.isDirectory()) {
        return -2;
    }

    int count = 0;
    File file = root.openNextFile();
    while(file) {
        const char * name = file.name();
        bool is_dir = file.isDirectory();
        uint32_t size = is_dir ? 0 : file.size();
        if(cb) cb(name, is_dir, size, ctx);
        file = root.openNextFile();
        count++;
    }
    return count;
}

int sd_card_read_file(const char * path, uint8_t * buffer, int max_len) {
    if(!sd_initialized) {
        return -1;
    }

    File file = SD.open(path, FILE_READ);
    if(!file) {
        Serial.print("Failed to open file: ");
        Serial.println(path);
        return -1;
    }

    int bytes_read = file.read(buffer, max_len);
    file.close();
    return bytes_read;
}

bool sd_card_write_file(const char * path, const uint8_t * data, int len) {
    if(!sd_initialized) {
        return false;
    }

    File file = SD.open(path, FILE_WRITE);
    if(!file) {
        Serial.print("Failed to open file for writing: ");
        Serial.println(path);
        return false;
    }

    int bytes_written = file.write(data, len);
    file.close();
    return (bytes_written == len);
}

bool sd_card_file_exists(const char * path) {
    if(!sd_initialized) {
        return false;
    }

    return SD.exists(path);
}

bool sd_card_delete_file(const char * path) {
    if(!sd_initialized) {
        return false;
    }

    return SD.remove(path);
}

uint64_t sd_card_get_free_space(void) {
    if(!sd_initialized) {
        return 0;
    }

    /*Note: ESP32 SD library doesn't provide direct free space API*/
    /*This is a workaround - returns total card size*/
    return SD.cardSize();
}

bool sd_card_format(void) {
    if(!sd_initialized) {
        /*Try to mount first*/
        if(!sd_card_init()) return false;
    }
    
    /*Unmount first*/
    SD.end();
    sd_initialized = false;
    
    /*Remount and format using FFAT/FATFS*/
    sdSPI.begin(SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);
    
    /*Try to format - Arduino SD library has format() in some versions*/
    #if defined(SD_FORMAT_SUPPORTED)
    if(!SD.format()) {
        Serial.println("SD format failed");
        return false;
    }
    #else
    /*Manual format: delete all files in root*/
    if(!SD.begin(SD_CS_PIN, sdSPI, 4000000)) {
        Serial.println("SD remount failed for format");
        return false;
    }
    sd_initialized = true;
    
    /*Delete all files and directories recursively*/
    File root = SD.open("/");
    if(root) {
        File file = root.openNextFile();
        while(file) {
            const char * name = file.name();
            bool isDir = file.isDirectory();
            file.close();
            
            char path[64];
            snprintf(path, sizeof(path), "/%s", name);
            
            if(isDir) {
                /*Recursively delete directory contents first*/
                File dir = SD.open(path);
                if(dir) {
                    File subfile = dir.openNextFile();
                    while(subfile) {
                        char subpath[128];
                        snprintf(subpath, sizeof(subpath), "%s/%s", path, subfile.name());
                        subfile.close();
                        SD.remove(subpath);
                        subfile = dir.openNextFile();
                    }
                    dir.close();
                }
                SD.rmdir(path);
            } else {
                SD.remove(path);
            }
            
            file = root.openNextFile();
        }
        root.close();
    }
    Serial.println("SD card formatted (all files deleted)");
    #endif
    
    return true;
}

/*Browse directory with full entries information*/
int sd_card_list_dir_browse(const char * path, sd_entry_t * entries, int max_entries) {
    if(!sd_initialized || !entries || max_entries <= 0) {
        return 0;
    }
    
    File root = SD.open(path);
    if(!root || !root.isDirectory()) {
        Serial.printf("[SD] Failed to open directory: %s\n", path);
        return 0;
    }
    
    int count = 0;
    File file = root.openNextFile();
    
    while(file && count < max_entries) {
        const char * name = file.name();
        bool is_dir = file.isDirectory();
        uint32_t size = file.size();
        
        /*Build full path*/
        char full_path[128];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, name);
        
        /*Remove leading slash if path is root*/
        if(strcmp(path, "/") == 0) {
            snprintf(full_path, sizeof(full_path), "/%s", name);
        }
        
        /*Fill entry*/
        strncpy(entries[count].name, name, sizeof(entries[count].name) - 1);
        entries[count].name[sizeof(entries[count].name) - 1] = '\0';
        entries[count].is_directory = is_dir;
        entries[count].size = size;
        strncpy(entries[count].full_path, full_path, sizeof(entries[count].full_path) - 1);
        entries[count].full_path[sizeof(entries[count].full_path) - 1] = '\0';
        
        count++;
        file = root.openNextFile();
    }
    
    root.close();
    return count;
}
