#ifndef SD_CARD_H
#define SD_CARD_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/*SD Card initialization and file operations*/

/*Initialize SD card with SPI pins*/
bool sd_card_init(void);

/*Check if SD card is mounted*/
bool sd_card_is_mounted(void);

/*Unmount SD card*/
bool sd_card_unmount(void);

/*List files in directory*/
void sd_card_list_dir(const char * path);

/*Iterate directory entries via callback: name, is_dir, size*/
int sd_card_list_dir_iter(const char * path, void (*cb)(const char * name, bool is_dir, uint32_t size, void * ctx), void * ctx);

/*Read file content*/
int sd_card_read_file(const char * path, uint8_t * buffer, int max_len);

/*Write file content*/
bool sd_card_write_file(const char * path, const uint8_t * data, int len);

/*Check if file exists*/
bool sd_card_file_exists(const char * path);

/*Delete file*/
bool sd_card_delete_file(const char * path);

/*Get free space in bytes*/
uint64_t sd_card_get_free_space(void);

/*Format SD card (quick format)*/
bool sd_card_format(void);

/*Browse directory - improved listing with full paths*/
typedef struct {
    char name[64];          /* Filename or dirname */
    bool is_directory;      /* True if directory */
    uint32_t size;          /* File size in bytes */
    char full_path[128];    /* Full path to file/dir */
} sd_entry_t;

int sd_card_list_dir_browse(const char * path, sd_entry_t * entries, int max_entries);

#ifdef __cplusplus
}
#endif

#endif // SD_CARD_H
