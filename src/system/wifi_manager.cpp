#include "system/wifi_manager.h"
#include <WiFi.h>
#include <Preferences.h>
#include <Arduino.h>

static bool wifi_enabled = false;
static char ip_str_buf[32] = "0.0.0.0";
static Preferences prefs;

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

    /* Automatically save connected network into NVS */
    wifi_sys_save_network(clean_ssid, password);
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

/* NVS Persistent Storage Implementation (Max 3 Networks) */
int wifi_sys_get_saved_count(void) {
    prefs.begin("bms_wifi", true);
    int count = prefs.getInt("count", 0);
    prefs.end();
    if (count < 0) count = 0;
    if (count > 3) count = 3;
    return count;
}

bool wifi_sys_get_saved(int idx, char *ssid_out, size_t ssid_size, char *pass_out, size_t pass_size) {
    int count = wifi_sys_get_saved_count();
    if (idx < 0 || idx >= count) return false;

    prefs.begin("bms_wifi", true);
    char k_ssid[16], k_pass[16];
    snprintf(k_ssid, sizeof(k_ssid), "s_%d", idx);
    snprintf(k_pass, sizeof(k_pass), "p_%d", idx);

    String s = prefs.getString(k_ssid, "");
    String p = prefs.getString(k_pass, "");
    prefs.end();

    if (s.length() == 0) return false;

    if (ssid_out && ssid_size > 0) {
        strncpy(ssid_out, s.c_str(), ssid_size - 1);
        ssid_out[ssid_size - 1] = '\0';
    }
    if (pass_out && pass_size > 0) {
        strncpy(pass_out, p.c_str(), pass_size - 1);
        pass_out[pass_size - 1] = '\0';
    }
    return true;
}

bool wifi_sys_save_network(const char *ssid, const char *password) {
    if (!ssid || strlen(ssid) == 0) return false;
    if (strcmp(ssid, "[ Manual Input ]") == 0 || strcmp(ssid, "[ No Networks Found ]") == 0) return false;

    int count = wifi_sys_get_saved_count();

    /* Check if SSID already exists */
    for (int i = 0; i < count; i++) {
        char s_curr[64], p_curr[64];
        if (wifi_sys_get_saved(i, s_curr, sizeof(s_curr), p_curr, sizeof(p_curr))) {
            if (strcmp(s_curr, ssid) == 0) {
                /* Update password for existing slot */
                prefs.begin("bms_wifi", false);
                char k_pass[16];
                snprintf(k_pass, sizeof(k_pass), "p_%d", i);
                prefs.putString(k_pass, password ? password : "");
                prefs.end();
                return true;
            }
        }
    }

    prefs.begin("bms_wifi", false);

    if (count < 3) {
        char k_ssid[16], k_pass[16];
        snprintf(k_ssid, sizeof(k_ssid), "s_%d", count);
        snprintf(k_pass, sizeof(k_pass), "p_%d", count);
        prefs.putString(k_ssid, ssid);
        prefs.putString(k_pass, password ? password : "");
        prefs.putInt("count", count + 1);
    } else {
        /* Shift slots: 1->0, 2->1, new->2 */
        char s1[64], p1[64], s2[64], p2[64];
        wifi_sys_get_saved(1, s1, sizeof(s1), p1, sizeof(p1));
        wifi_sys_get_saved(2, s2, sizeof(s2), p2, sizeof(p2));

        prefs.putString("s_0", s1);
        prefs.putString("p_0", p1);
        prefs.putString("s_1", s2);
        prefs.putString("p_1", p2);
        prefs.putString("s_2", ssid);
        prefs.putString("p_2", password ? password : "");
        prefs.putInt("count", 3);
    }

    prefs.end();
    return true;
}

bool wifi_sys_delete_saved(int idx) {
    int count = wifi_sys_get_saved_count();
    if (idx < 0 || idx >= count) return false;

    prefs.begin("bms_wifi", false);

    /* Shift remaining slots up */
    for (int i = idx; i < count - 1; i++) {
        char s_next[64], p_next[64];
        wifi_sys_get_saved(i + 1, s_next, sizeof(s_next), p_next, sizeof(p_next));
        char k_s[16], k_p[16];
        snprintf(k_s, sizeof(k_s), "s_%d", i);
        snprintf(k_p, sizeof(k_p), "p_%d", i);
        prefs.putString(k_s, s_next);
        prefs.putString(k_p, p_next);
    }

    /* Delete last slot */
    char k_last_s[16], k_last_p[16];
    snprintf(k_last_s, sizeof(k_last_s), "s_%d", count - 1);
    snprintf(k_last_p, sizeof(k_last_p), "p_%d", count - 1);
    prefs.remove(k_last_s);
    prefs.remove(k_last_p);

    prefs.putInt("count", count - 1);
    prefs.end();
    return true;
}
