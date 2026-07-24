#include "system/wifi_manager.h"
#include <WiFi.h>
#include <Arduino.h>

static bool wifi_enabled = false;
static char ip_str_buf[32] = "0.0.0.0";

void wifi_sys_set_enabled(bool enabled) {
    wifi_enabled = enabled;
    if (enabled) {
        WiFi.mode(WIFI_STA);
        WiFi.setAutoReconnect(true);
    } else {
        WiFi.disconnect(true, true);
        WiFi.mode(WIFI_OFF);
    }
}

bool wifi_sys_is_enabled(void) {
    return wifi_enabled;
}

int wifi_sys_scan_networks(char *options_buf, size_t buf_size) {
    if (!wifi_enabled) {
        wifi_sys_set_enabled(true);
    }

    int n = WiFi.scanNetworks();
    if (n <= 0) {
        snprintf(options_buf, buf_size, "[ No Networks Found ]\n[ Manual Input ]");
        return 0;
    }

    options_buf[0] = '\0';
    size_t offset = 0;
    int count = 0;

    for (int i = 0; i < n && i < 15; ++i) {
        String ssid = WiFi.SSID(i);
        if (ssid.length() == 0) continue;

        int32_t rssi = WiFi.RSSI(i);
        char entry[64];
        snprintf(entry, sizeof(entry), "%s (%ddBm)%s", 
                 ssid.c_str(), 
                 (int)rssi, 
                 (i == n - 1 || i == 14) ? "" : "\n");

        size_t len = strlen(entry);
        if (offset + len < buf_size - 25) {
            strcat(options_buf, entry);
            offset += len;
            count++;
        }
    }

    if (offset + 18 < buf_size) {
        if (count > 0 && options_buf[strlen(options_buf) - 1] != '\n') {
            strcat(options_buf, "\n");
        }
        strcat(options_buf, "[ Manual Input ]");
    }

    WiFi.scanDelete();
    return count;
}

bool wifi_sys_connect(const char *ssid, const char *password) {
    if (!ssid || strlen(ssid) == 0) return false;

    if (!wifi_enabled) {
        wifi_sys_set_enabled(true);
    }

    /* Extract clean SSID if formatted like "MyWiFi (-65dBm)" */
    char clean_ssid[64];
    strncpy(clean_ssid, ssid, sizeof(clean_ssid) - 1);
    clean_ssid[sizeof(clean_ssid) - 1] = '\0';
    
    char *paren = strrchr(clean_ssid, '(');
    if (paren && paren > clean_ssid && *(paren - 1) == ' ') {
        *(paren - 1) = '\0';
    }

    if (strcmp(clean_ssid, "[ Manual Input ]") == 0 || strcmp(clean_ssid, "[ No Networks Found ]") == 0) {
        return false;
    }

    WiFi.begin(clean_ssid, password);
    return true;
}

void wifi_sys_disconnect(void) {
    WiFi.disconnect(false, false);
}

bool wifi_sys_is_connected(void) {
    return (WiFi.status() == WL_CONNECTED);
}

const char* wifi_sys_get_ip_str(void) {
    if (WiFi.status() == WL_CONNECTED) {
        snprintf(ip_str_buf, sizeof(ip_str_buf), "%s", WiFi.localIP().toString().c_str());
    } else {
        snprintf(ip_str_buf, sizeof(ip_str_buf), "0.0.0.0");
    }
    return ip_str_buf;
}
