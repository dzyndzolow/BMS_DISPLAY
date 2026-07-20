# SD Card Configuration (JC4827W543)

## Hardware Pins (TF slot)
- CS: GPIO10 (TF_CS)
- MOSI: GPIO11 (TF_MOSI)
- MISO: GPIO13 (TF_MISO)
- SCK: GPIO12 (TF_CLK)
- Bus: HSPI dedicated instance
- Default speed: 4 MHz

## Firmware Modules
- `src/sd_card.cpp`: low-level SD + listing helpers
- `src/logger.cpp`: file logging to `/logs` with rotation
- `src/lv_demo_widgets.c`: UI tab (Files/Tools) + auto-mount + browser

## Auto-Mount
- On SD tab creation: attempts `sd_card_init()` once.
- On success: calls `logger_init()` and refreshes list.
- Manual mount/unmount still available via button.

## UI Layout (SD Card tab)
- Left panel Files: 60% width; right panel Tools: 40%.
- Files panel: clickable folders/files, parent `..`, shows sizes.
- Tools: Mount/Unmount, Refresh, Format (quick delete), Test Log.

## Logger Behavior
- Logs stored under `/logs/log_*.txt` with rotation (5 files, 512 KB each).
- Test Log button writes INFO/DEBUG/WARN entries and flushes immediately.
- `logger_init()` runs after SD is mounted (auto or manual).

## Usage Notes
- Card must be FAT32 formatted.
- If boot mount fails (timing), open SD tab and press Mount (will init logger).
- Format button deletes all files on the card.

## Build/Ports
- Upload: COM7 @ 921600
- Monitor: COM8 @ 115200

## Key APIs
- Init: `sd_card_init()`
- Mounted check: `sd_card_is_mounted()`
- Directory browse: `sd_card_list_dir_browse(path, entries, max_entries)`
- Logger init: `logger_init()`
- Write log: `log_info/debug/warn/error/fatal(...)`
