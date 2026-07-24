#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void wifi_sys_set_enabled(bool enabled);
bool wifi_sys_is_enabled(void);

/* Performs real WiFi scan and populates options_buf ("SSID1\nSSID2\n...") */
int wifi_sys_scan_networks(char *options_buf, size_t buf_size);

/* Starts connection to real WiFi network */
bool wifi_sys_connect(const char *ssid, const char *password);

/* Disconnects real WiFi */
void wifi_sys_disconnect(void);

/* Returns true if WL_CONNECTED */
bool wifi_sys_is_connected(void);

/* Returns current IP address string e.g. "192.168.1.105" */
const char* wifi_sys_get_ip_str(void);

#ifdef __cplusplus
}
#endif

#endif // WIFI_MANAGER_H
