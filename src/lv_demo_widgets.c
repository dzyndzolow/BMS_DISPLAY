/**
 * @file lv_demo_widgets.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "lv_demo_widgets.h"
#include "logger.h"
#include "sd_card.h"
#include <esp_system.h>
#include <stdio.h>
#include <string.h>

/*Backlight control - defined in LvglWidgets.cpp*/
extern void backlight_set(uint8_t percent);

#if LV_USE_DEMO_WIDGETS

#if LV_MEM_CUSTOM == 0 && LV_MEM_SIZE < (38ul * 1024ul)
#error Insufficient memory for lv_demo_widgets. Please set LV_MEM_SIZE to at least 38KB (38ul * 1024ul).  48KB is recommended.
#endif

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/
typedef enum {
  DISP_SMALL,
  DISP_MEDIUM,
  DISP_LARGE,
} disp_size_t;

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void profile_create(lv_obj_t *parent);
static void analytics_create(lv_obj_t *parent);
static void shop_create(lv_obj_t *parent);
static void color_changer_create(lv_obj_t *parent);
static void settings_create(lv_obj_t *parent);
static void network_create(lv_obj_t *parent);
static void sd_card_tab_create(lv_obj_t *parent);

static lv_obj_t *create_meter_box(lv_obj_t *parent, const char *title,
                                  const char *text1, const char *text2,
                                  const char *text3);
static lv_obj_t *create_shop_item(lv_obj_t *parent, const void *img_src,
                                  const char *name, const char *category,
                                  const char *price);

static void color_changer_event_cb(lv_event_t *e);
static void color_event_cb(lv_event_t *e);
static void net_ta_event_cb(lv_event_t *e);
static void ta_event_cb(lv_event_t *e);
static void backlight_slider_cb(lv_event_t *e);
static void reset_btn_event_cb(lv_event_t *e);
#if LV_USE_PERF_MONITOR
static void perf_monitor_switch_cb(lv_event_t *e);
static lv_obj_t *perf_monitor_find_label(void);
static void perf_monitor_init_cb(lv_timer_t *t);
#endif
static void birthday_event_cb(lv_event_t *e);
static void calendar_event_cb(lv_event_t *e);
static void slider_event_cb(lv_event_t *e);
static void chart_event_cb(lv_event_t *e);
static void shop_chart_event_cb(lv_event_t *e);
static void meter1_indic1_anim_cb(void *var, int32_t v);
static void meter1_indic2_anim_cb(void *var, int32_t v);
static void meter1_indic3_anim_cb(void *var, int32_t v);
static void meter2_timer_cb(lv_timer_t *timer);
static void profile_soc_timer_cb(lv_timer_t *timer);
static void meter3_anim_cb(void *var, int32_t v);
static void sd_mount_btn_event_cb(lv_event_t *e);
static void sd_format_btn_event_cb(lv_event_t *e);
static void sd_test_log_btn_event_cb(lv_event_t *e);
static void sd_refresh_btn_event_cb(lv_event_t *e);
static void sd_refresh_list(void);
static void sd_list_iter_cb(const char *name, bool is_dir, uint32_t size,
                            void *ctx);
static void sd_list_item_cb(lv_event_t *e);
static void sd_list_parent_cb(lv_event_t *e);
static void bottom_nav_btn_cb(lv_event_t *e);
static void bottom_nav_create(void);

/**********************
 *  STATIC VARIABLES
 **********************/
static disp_size_t disp_size;
static lv_obj_t *tv;
static lv_obj_t *status_info_cont = NULL;
static lv_obj_t *calendar;
static lv_obj_t *bottom_nav_btns[5];
static lv_style_t style_text_muted;
static lv_style_t style_title;
static lv_style_t style_icon;
static lv_style_t style_bullet;

static lv_color_t current_accent_color;
static lv_obj_t *profile_panel1 = NULL;
static lv_obj_t *profile_avatar = NULL;
static lv_obj_t *profile_pct_label = NULL;
static lv_obj_t *profile_sub_box_val_labels[6] = {NULL};
static lv_obj_t *profile_sub_boxes[6] = {NULL};
static int32_t profile_soc_val = 70;
static bool profile_soc_down = true;
static lv_timer_t *profile_soc_timer = NULL;
static lv_obj_t *settings_panel = NULL;
static lv_obj_t *network_panel = NULL;
static lv_obj_t *reset_btn = NULL;
static lv_obj_t *sd_left_panel = NULL;
static lv_obj_t *sd_right_panel = NULL;
static lv_obj_t *color_changer_cont = NULL;
static lv_obj_t *color_changer_title_cont = NULL;
static lv_obj_t *color_changer_swatches_cont = NULL;

static lv_obj_t *net_wifi_status_label = NULL;
static lv_obj_t *net_wifi_dropdown = NULL;
static lv_obj_t *net_wifi_pass_ta = NULL;
static lv_obj_t *net_mqtt_status_label = NULL;
static lv_obj_t *net_ha_disc_status_label = NULL;
static lv_obj_t *net_ip_mode_dropdown = NULL;
static lv_obj_t *net_ip_static_cont = NULL;

static const uint32_t custom_colors[7] = {
    0x1D70D8, /* Electric Blue (Default) */
    0x00E5FF, /* Neon Cyan */
    0x10B981, /* Emerald Green */
    0x8B5CF6, /* Vivid Purple */
    0xF59E0B, /* Amber Orange */
    0xEF4444, /* Crimson Red */
    0xEC4899  /* Hot Pink */
};

static void update_accent_color(lv_color_t color);

static lv_obj_t *meter1;
static lv_obj_t *meter2;
static lv_obj_t *meter3;

static lv_obj_t *chart1;
static lv_obj_t *chart2;
static lv_obj_t *chart3;

static lv_chart_series_t *ser1;
static lv_chart_series_t *ser2;
static lv_chart_series_t *ser3;
static lv_chart_series_t *ser4;

static const lv_font_t *font_large;
static const lv_font_t *font_normal;

static lv_obj_t *sd_list = NULL;
static lv_obj_t *sd_status_label = NULL;
static lv_obj_t *sd_mount_btn = NULL;
static lv_obj_t *sd_mount_label = NULL;
static lv_obj_t *sd_format_btn = NULL;

/*SD Card browser state*/
static char sd_current_path[128] = "/";
static sd_entry_t sd_entries[32]; /* Max 32 entries per folder */
static int sd_entry_count = 0;

static uint32_t session_desktop = 1000;
static uint32_t session_tablet = 1000;
static uint32_t session_mobile = 1000;

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void lv_demo_widgets(void) {
  if (LV_HOR_RES <= 320)
    disp_size = DISP_SMALL;
  else if (LV_HOR_RES < 720)
    disp_size = DISP_MEDIUM;
  else
    disp_size = DISP_LARGE;

  font_large = LV_FONT_DEFAULT;
  font_normal = LV_FONT_DEFAULT;

  lv_coord_t tab_h;
  if (disp_size == DISP_LARGE) {
    tab_h = 55;
#if LV_FONT_MONTSERRAT_24
    font_large = &lv_font_montserrat_24;
#else
    LV_LOG_WARN("LV_FONT_MONTSERRAT_24 is not enabled for the widgets demo. "
                "Using LV_FONT_DEFAULT instead.");
#endif
#if LV_FONT_MONTSERRAT_16
    font_normal = &lv_font_montserrat_16;
#else
    LV_LOG_WARN("LV_FONT_MONTSERRAT_16 is not enabled for the widgets demo. "
                "Using LV_FONT_DEFAULT instead.");
#endif
  } else if (disp_size == DISP_MEDIUM) {
    tab_h = 36;
#if LV_FONT_MONTSERRAT_16
    font_large = &lv_font_montserrat_16;
#else
    font_large = LV_FONT_DEFAULT;
#endif
#if LV_FONT_MONTSERRAT_14
    font_normal = &lv_font_montserrat_14;
#else
    font_normal = LV_FONT_DEFAULT;
#endif
  } else { /* disp_size == DISP_SMALL */
    tab_h = 36;
#if LV_FONT_MONTSERRAT_18
    font_large = &lv_font_montserrat_18;
#else
    LV_LOG_WARN("LV_FONT_MONTSERRAT_18 is not enabled for the widgets demo. "
                "Using LV_FONT_DEFAULT instead.");
#endif
#if LV_FONT_MONTSERRAT_12
    font_normal = &lv_font_montserrat_12;
#else
    LV_LOG_WARN("LV_FONT_MONTSERRAT_12 is not enabled for the widgets demo. "
                "Using LV_FONT_DEFAULT instead.");
#endif
  }

#if LV_USE_THEME_DEFAULT
  lv_theme_default_init(NULL, lv_color_hex(0x1D70D8), lv_color_hex(0x38BDF8),
                        true, font_normal);
#endif

  current_accent_color = lv_color_hex(0x1D70D8);
  lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x121A28), 0);
  lv_obj_set_style_bg_grad_color(lv_scr_act(), lv_color_hex(0x05070B), 0);
  lv_obj_set_style_bg_grad_dir(lv_scr_act(), LV_GRAD_DIR_VER, 0);
  lv_obj_set_style_bg_opa(lv_scr_act(), LV_OPA_COVER, 0);

  lv_style_init(&style_text_muted);
  lv_style_set_text_color(&style_text_muted, lv_color_hex(0x566D85));
  lv_style_set_text_opa(&style_text_muted, LV_OPA_90);
  lv_style_set_text_font(&style_text_muted, font_normal);

  lv_style_init(&style_title);
  lv_style_set_text_color(&style_title, lv_color_hex(0xFFFFFF));
  lv_style_set_text_font(&style_title, font_large);

  lv_style_init(&style_icon);
  lv_style_set_text_color(&style_icon, lv_color_hex(0x1D70D8));
  lv_style_set_text_font(&style_icon, font_large);

  lv_style_init(&style_bullet);
  lv_style_set_border_width(&style_bullet, 0);
  lv_style_set_radius(&style_bullet, LV_RADIUS_CIRCLE);

  tv = lv_tabview_create(lv_scr_act(), LV_DIR_TOP, 0);
  lv_obj_set_pos(tv, 0, 31);
  lv_obj_set_size(tv, LV_PCT(100), LV_VER_RES - 62);
  lv_obj_set_style_bg_color(tv, lv_color_hex(0x121A28), 0);
  lv_obj_set_style_bg_grad_color(tv, lv_color_hex(0x05070B), 0);
  lv_obj_set_style_bg_grad_dir(tv, LV_GRAD_DIR_VER, 0);
  lv_obj_set_style_bg_opa(tv, LV_OPA_COVER, 0);

  lv_obj_t *tab_btns = lv_tabview_get_tab_btns(tv);
  lv_obj_add_flag(tab_btns, LV_OBJ_FLAG_HIDDEN); /* Collapsed by default */

  /* Dedicated Top Status Bar Container (Pinned to Top) */
  lv_obj_t *status_bar = lv_obj_create(lv_scr_act());
  lv_obj_set_size(status_bar, LV_PCT(100), 31);
  lv_obj_align(status_bar, LV_ALIGN_TOP_MID, 0, 0);
  lv_obj_set_style_pad_all(status_bar, 0, 0);
  lv_obj_set_style_bg_color(status_bar, lv_color_hex(0x0E1420), 0);
  lv_obj_set_style_bg_grad_color(status_bar, lv_color_hex(0x070B12), 0);
  lv_obj_set_style_bg_grad_dir(status_bar, LV_GRAD_DIR_VER, 0);
  lv_obj_set_style_bg_opa(status_bar, LV_OPA_COVER, 0);
  lv_obj_set_style_border_color(status_bar, lv_color_hex(0x1F2836), 0);
  lv_obj_set_style_border_side(status_bar, LV_BORDER_SIDE_BOTTOM, 0);
  lv_obj_set_style_border_width(status_bar, 1, 0);
  lv_obj_clear_flag(status_bar, LV_OBJ_FLAG_SCROLLABLE);

  /* Status Info Container (BP72 BMS title, centered date/time, wifi) */
  status_info_cont = lv_obj_create(status_bar);
  lv_obj_set_size(status_info_cont, LV_PCT(100), 31);
  lv_obj_align(status_info_cont, LV_ALIGN_TOP_MID, 0, 0);
  lv_obj_set_style_pad_all(status_info_cont, 0, 0);
  lv_obj_set_style_bg_opa(status_info_cont, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(status_info_cont, 0, 0);
  lv_obj_clear_flag(status_info_cont, LV_OBJ_FLAG_SCROLLABLE);

  /* BP72 BMS Title */
  lv_obj_t *bms_title = lv_label_create(status_info_cont);
  lv_label_set_text(bms_title, "BP72 BMS");
  lv_obj_add_style(bms_title, &style_title, 0);
  lv_obj_align(bms_title, LV_ALIGN_LEFT_MID, 10, 0);

  /* Centered Date & Time */
  lv_obj_t *datetime_lbl = lv_label_create(status_info_cont);
  lv_label_set_text(datetime_lbl, "2026-07-23 19:02");
  lv_obj_add_style(datetime_lbl, &style_text_muted, 0);
  lv_obj_align(datetime_lbl, LV_ALIGN_CENTER, 0, 0);

  /* WiFi Icon */
  lv_obj_t *wifi_icon = lv_label_create(status_info_cont);
  lv_label_set_text(wifi_icon, LV_SYMBOL_WIFI);
  lv_obj_add_style(wifi_icon, &style_icon, 0);
  lv_obj_align(wifi_icon, LV_ALIGN_RIGHT_MID, -6, 0);

  lv_obj_set_style_text_font(lv_scr_act(), font_normal, 0);

  lv_obj_t *t1 = lv_tabview_add_tab(tv, "Profile");
  lv_obj_t *t2 = lv_tabview_add_tab(tv, "Analytics");
  lv_obj_t *t3 = lv_tabview_add_tab(tv, "Summary");
  lv_obj_t *t4 = lv_tabview_add_tab(tv, "Settings");
  lv_obj_t *t5 = lv_tabview_add_tab(tv, "Network");
  lv_obj_t *t6 = lv_tabview_add_tab(tv, "SD Card");

  lv_obj_t *all_tabs[] = {t1, t2, t3, t4, t5, t6};
  for (int i = 0; i < 6; i++) {
    lv_obj_set_style_bg_color(all_tabs[i], lv_color_hex(0x121A28), 0);
    lv_obj_set_style_bg_grad_color(all_tabs[i], lv_color_hex(0x05070B), 0);
    lv_obj_set_style_bg_grad_dir(all_tabs[i], LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_bg_opa(all_tabs[i], LV_OPA_COVER, 0);
  }
  profile_create(t1);
  analytics_create(t2);
  shop_create(t3);
  settings_create(t4);
  network_create(t5);
  sd_card_tab_create(t6);
  bottom_nav_create(); /* Permanent bottom navigation bar */
}

/**********************
 *   STATIC FUNCTIONS
 **********************/
static void settings_create(lv_obj_t *parent) {
  lv_obj_t *panel = lv_obj_create(parent);
  settings_panel = panel;
  lv_obj_set_height(panel, LV_SIZE_CONTENT);
  lv_obj_set_width(panel, lv_pct(100));
  lv_obj_set_style_pad_all(panel, 15, 0);
  lv_obj_set_style_bg_color(panel, lv_color_hex(0x111622), 0);
  lv_obj_set_style_border_color(panel, current_accent_color, 0);
  lv_obj_set_style_border_width(panel, 1, 0);
  lv_obj_set_flex_flow(panel, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(panel, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START,
                        LV_FLEX_ALIGN_START);

  lv_obj_t *title = lv_label_create(panel);
  lv_label_set_text(title, "Display Settings");
  lv_obj_add_style(title, &style_title, 0);

  /* System Color Manager placed right under Display Settings title */
  color_changer_create(panel);

#if LV_USE_PERF_MONITOR
  /*Performance Monitor toggle*/
  lv_obj_t *perf_cont = lv_obj_create(panel);
  lv_obj_set_size(perf_cont, lv_pct(100), LV_SIZE_CONTENT);
  lv_obj_set_style_pad_all(perf_cont, 10, 0);
  lv_obj_clear_flag(perf_cont, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_flex_flow(perf_cont, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(perf_cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);

  lv_obj_t *perf_chk = lv_checkbox_create(perf_cont);
  lv_checkbox_set_text(perf_chk, "Performance Monitor (FPS/CPU)");
  lv_obj_add_event_cb(perf_chk, perf_monitor_switch_cb, LV_EVENT_VALUE_CHANGED,
                      NULL);
  /* Default OFF - sync label visibility after initial render */
  lv_obj_clear_state(perf_chk, LV_STATE_CHECKED);
  lv_timer_create(perf_monitor_init_cb, 100, perf_chk);
#endif

  /*Backlight brightness slider*/
  lv_obj_t *bl_cont = lv_obj_create(panel);
  lv_obj_set_size(bl_cont, lv_pct(100), LV_SIZE_CONTENT);
  lv_obj_set_style_pad_all(bl_cont, 10, 0);
  lv_obj_clear_flag(bl_cont, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_flex_flow(bl_cont, LV_FLEX_FLOW_COLUMN);

  lv_obj_t *bl_label = lv_label_create(bl_cont);
  lv_label_set_text(bl_label, "Backlight Brightness");

  lv_obj_t *bl_slider = lv_slider_create(bl_cont);
  lv_obj_set_width(bl_slider, lv_pct(90));
  lv_slider_set_range(bl_slider, 1, 100); /*min 5% to avoid black screen*/
  lv_slider_set_value(bl_slider, 100, LV_ANIM_OFF);
  lv_obj_add_event_cb(bl_slider, backlight_slider_cb, LV_EVENT_VALUE_CHANGED,
                      NULL);
  lv_obj_add_event_cb(bl_slider, backlight_slider_cb, LV_EVENT_ALL, NULL);

  /*Reset device button in its own row (half width)*/
  lv_obj_t *reset_row = lv_obj_create(panel);
  lv_obj_set_width(reset_row, lv_pct(100));
  lv_obj_set_style_pad_all(reset_row, 0, 0);
  lv_obj_set_style_border_width(reset_row, 0, 0);
  lv_obj_set_style_bg_opa(reset_row, LV_OPA_TRANSP, 0);
  lv_obj_set_flex_flow(reset_row, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(reset_row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);

  lv_obj_t *reset_btn_obj = lv_btn_create(reset_row);
  reset_btn = reset_btn_obj;
  lv_obj_set_width(reset_btn,
                   lv_pct(100)); /*Full width since no Rotate button*/
  lv_obj_set_style_pad_all(reset_btn, 12, 0);
  lv_obj_set_style_bg_color(reset_btn, current_accent_color, 0);
  lv_obj_t *reset_label = lv_label_create(reset_btn);
  lv_label_set_text(reset_label, "Reset Device");
  lv_obj_center(reset_label);
  lv_obj_add_event_cb(reset_btn, reset_btn_event_cb, LV_EVENT_CLICKED, NULL);
}

static lv_obj_t *net_kb = NULL;

static void net_ta_event_cb(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t *ta = lv_event_get_target(e);

  if (code == LV_EVENT_FOCUSED) {
    if (net_kb == NULL) {
      net_kb = lv_keyboard_create(lv_scr_act());
      lv_obj_set_style_bg_color(net_kb, lv_color_hex(0x111622), 0);
      lv_obj_set_style_border_color(net_kb, current_accent_color, 0);
      lv_obj_set_style_border_width(net_kb, 1, 0);
    }
    lv_keyboard_set_textarea(net_kb, ta);
    lv_obj_clear_flag(net_kb, LV_OBJ_FLAG_HIDDEN);
  } else if (code == LV_EVENT_DEFOCUSED || code == LV_EVENT_READY) {
    if (net_kb) {
      lv_obj_add_flag(net_kb, LV_OBJ_FLAG_HIDDEN);
    }
  }
}

static void wifi_scan_btn_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;

  if (net_wifi_status_label) {
    lv_label_set_text(net_wifi_status_label, "Status: Scanning networks...");
  }
  if (net_wifi_dropdown) {
    lv_dropdown_set_options(net_wifi_dropdown,
      "BMS_Home_Network\nBMS_Workshop_5G\nESP32_Access_Point\nGuest_WiFi\n[ Manual Entry ]");
  }
  if (net_wifi_status_label) {
    lv_label_set_text(net_wifi_status_label, "Status: Scan complete (4 networks found)");
  }
}

static void wifi_connect_btn_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;

  if (net_wifi_status_label) {
    lv_label_set_text(net_wifi_status_label, "Status: Connected (192.168.1.105)");
  }
}

static void wifi_disconnect_btn_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;

  if (net_wifi_status_label) {
    lv_label_set_text(net_wifi_status_label, "Status: Disconnected");
  }
}

static void ip_mode_event_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) return;

  if (net_ip_mode_dropdown && net_ip_static_cont) {
    uint16_t sel = lv_dropdown_get_selected(net_ip_mode_dropdown);
    if (sel == 1) { /* Static IP selected */
      lv_obj_clear_flag(net_ip_static_cont, LV_OBJ_FLAG_HIDDEN);
    } else { /* DHCP selected */
      lv_obj_add_flag(net_ip_static_cont, LV_OBJ_FLAG_HIDDEN);
    }
  }
}

static void mqtt_connect_btn_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;

  if (net_mqtt_status_label) {
    lv_label_set_text(net_mqtt_status_label, "MQTT Status: Connected (broker.home:1883)");
  }
}

static void ha_discovery_btn_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;

  if (net_ha_disc_status_label) {
    lv_label_set_text(net_ha_disc_status_label, "HA Auto-Discovery payload published! (14 entities)");
  }
}

static void network_create(lv_obj_t *parent) {
  lv_obj_t *panel = lv_obj_create(parent);
  network_panel = panel;
  lv_obj_set_height(panel, LV_SIZE_CONTENT);
  lv_obj_set_width(panel, lv_pct(100));
  lv_obj_set_style_pad_all(panel, 12, 0);
  lv_obj_set_style_bg_color(panel, lv_color_hex(0x111622), 0);
  lv_obj_set_style_border_color(panel, current_accent_color, 0);
  lv_obj_set_style_border_width(panel, 1, 0);
  lv_obj_set_flex_flow(panel, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(panel, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START,
                        LV_FLEX_ALIGN_START);
  lv_obj_set_style_pad_row(panel, 10, 0);

  lv_obj_t *title = lv_label_create(panel);
  lv_label_set_text(title, "Network & Connectivity");
  lv_obj_add_style(title, &style_title, 0);

  /* --- SECTION 1: WiFi Manager --- */
  lv_obj_t *wifi_box = lv_obj_create(panel);
  lv_obj_set_size(wifi_box, lv_pct(100), LV_SIZE_CONTENT);
  lv_obj_set_style_pad_all(wifi_box, 10, 0);
  lv_obj_set_style_bg_color(wifi_box, lv_color_hex(0x161D2B), 0);
  lv_obj_set_style_border_color(wifi_box, lv_color_hex(0x1F2836), 0);
  lv_obj_set_style_border_width(wifi_box, 1, 0);
  lv_obj_set_style_radius(wifi_box, 8, 0);
  lv_obj_set_flex_flow(wifi_box, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_row(wifi_box, 6, 0);

  /* Title + Scan Button Row */
  lv_obj_t *w_hdr = lv_obj_create(wifi_box);
  lv_obj_remove_style_all(w_hdr);
  lv_obj_set_size(w_hdr, lv_pct(100), LV_SIZE_CONTENT);
  lv_obj_set_flex_flow(w_hdr, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(w_hdr, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

  lv_obj_t *w_lbl = lv_label_create(w_hdr);
  lv_label_set_text(w_lbl, LV_SYMBOL_WIFI " WiFi Manager");
  lv_obj_add_style(w_lbl, &style_title, 0);

  lv_obj_t *scan_btn = lv_btn_create(w_hdr);
  lv_obj_set_style_pad_ver(scan_btn, 6, 0);
  lv_obj_set_style_pad_hor(scan_btn, 10, 0);
  lv_obj_t *scan_lbl = lv_label_create(scan_btn);
  lv_label_set_text(scan_lbl, LV_SYMBOL_REFRESH " Scan");
  lv_obj_add_event_cb(scan_btn, wifi_scan_btn_cb, LV_EVENT_CLICKED, NULL);

  /* Status Label */
  net_wifi_status_label = lv_label_create(wifi_box);
  lv_label_set_text(net_wifi_status_label, "Status: Not connected");
  lv_obj_add_style(net_wifi_status_label, &style_text_muted, 0);

  /* Network SSID Dropdown */
  lv_obj_t *ssid_lbl = lv_label_create(wifi_box);
  lv_label_set_text(ssid_lbl, "Select Network (SSID)");
  lv_obj_add_style(ssid_lbl, &style_text_muted, 0);

  net_wifi_dropdown = lv_dropdown_create(wifi_box);
  lv_obj_set_width(net_wifi_dropdown, lv_pct(100));
  lv_dropdown_set_options(net_wifi_dropdown, "BMS_Home_Network\nESP32_Access_Point\n[ Manual Input ]");

  /* WiFi Password Input */
  lv_obj_t *pass_lbl = lv_label_create(wifi_box);
  lv_label_set_text(pass_lbl, "WiFi Password");
  lv_obj_add_style(pass_lbl, &style_text_muted, 0);

  net_wifi_pass_ta = lv_textarea_create(wifi_box);
  lv_obj_set_width(net_wifi_pass_ta, lv_pct(100));
  lv_textarea_set_one_line(net_wifi_pass_ta, true);
  lv_textarea_set_password_mode(net_wifi_pass_ta, true);
  lv_textarea_set_placeholder_text(net_wifi_pass_ta, "Enter password...");
  lv_obj_add_event_cb(net_wifi_pass_ta, net_ta_event_cb, LV_EVENT_ALL, NULL);

  /* Action Buttons (Connect / Disconnect) */
  lv_obj_t *w_act_row = lv_obj_create(wifi_box);
  lv_obj_remove_style_all(w_act_row);
  lv_obj_set_size(w_act_row, lv_pct(100), LV_SIZE_CONTENT);
  lv_obj_set_flex_flow(w_act_row, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(w_act_row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_column(w_act_row, 10, 0);

  lv_obj_t *conn_btn = lv_btn_create(w_act_row);
  lv_obj_set_flex_grow(conn_btn, 1);
  lv_obj_set_style_bg_color(conn_btn, current_accent_color, 0);
  lv_obj_t *conn_lbl = lv_label_create(conn_btn);
  lv_label_set_text(conn_lbl, LV_SYMBOL_OK " Connect");
  lv_obj_center(conn_lbl);
  lv_obj_add_event_cb(conn_btn, wifi_connect_btn_cb, LV_EVENT_CLICKED, NULL);

  lv_obj_t *disconn_btn = lv_btn_create(w_act_row);
  lv_obj_set_flex_grow(disconn_btn, 1);
  lv_obj_set_style_bg_color(disconn_btn, lv_color_hex(0x374151), 0);
  lv_obj_t *disconn_lbl = lv_label_create(disconn_btn);
  lv_label_set_text(disconn_lbl, LV_SYMBOL_CLOSE " Disconnect");
  lv_obj_center(disconn_lbl);
  lv_obj_add_event_cb(disconn_btn, wifi_disconnect_btn_cb, LV_EVENT_CLICKED, NULL);

  /* --- SECTION 2: IPv4 Configuration --- */
  lv_obj_t *ip_box = lv_obj_create(panel);
  lv_obj_set_size(ip_box, lv_pct(100), LV_SIZE_CONTENT);
  lv_obj_set_style_pad_all(ip_box, 10, 0);
  lv_obj_set_style_bg_color(ip_box, lv_color_hex(0x161D2B), 0);
  lv_obj_set_style_border_color(ip_box, lv_color_hex(0x1F2836), 0);
  lv_obj_set_style_border_width(ip_box, 1, 0);
  lv_obj_set_style_radius(ip_box, 8, 0);
  lv_obj_set_flex_flow(ip_box, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_row(ip_box, 6, 0);

  lv_obj_t *ip_title = lv_label_create(ip_box);
  lv_label_set_text(ip_title, LV_SYMBOL_SETTINGS " IPv4 Configuration");
  lv_obj_add_style(ip_title, &style_title, 0);

  lv_obj_t *ip_mode_lbl = lv_label_create(ip_box);
  lv_label_set_text(ip_mode_lbl, "IP Assignment Mode");
  lv_obj_add_style(ip_mode_lbl, &style_text_muted, 0);

  net_ip_mode_dropdown = lv_dropdown_create(ip_box);
  lv_obj_set_width(net_ip_mode_dropdown, lv_pct(100));
  lv_dropdown_set_options(net_ip_mode_dropdown, "DHCP (Dynamic IP)\nStatic IP Address");
  lv_obj_add_event_cb(net_ip_mode_dropdown, ip_mode_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

  /* Static IP Fields Container */
  net_ip_static_cont = lv_obj_create(ip_box);
  lv_obj_remove_style_all(net_ip_static_cont);
  lv_obj_set_size(net_ip_static_cont, lv_pct(100), LV_SIZE_CONTENT);
  lv_obj_set_flex_flow(net_ip_static_cont, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_row(net_ip_static_cont, 6, 0);
  lv_obj_add_flag(net_ip_static_cont, LV_OBJ_FLAG_HIDDEN); /* Hidden when DHCP selected */

  static const char *ip_field_names[4] = {"IP Address", "Subnet Mask", "Gateway", "Primary DNS"};
  static const char *ip_field_placeholders[4] = {"192.168.1.150", "255.255.255.0", "192.168.1.1", "8.8.8.8"};

  for (int i = 0; i < 4; i++) {
    lv_obj_t *f_lbl = lv_label_create(net_ip_static_cont);
    lv_label_set_text(f_lbl, ip_field_names[i]);
    lv_obj_add_style(f_lbl, &style_text_muted, 0);

    lv_obj_t *f_ta = lv_textarea_create(net_ip_static_cont);
    lv_obj_set_width(f_ta, lv_pct(100));
    lv_textarea_set_one_line(f_ta, true);
    lv_textarea_set_placeholder_text(f_ta, ip_field_placeholders[i]);
    lv_obj_add_event_cb(f_ta, net_ta_event_cb, LV_EVENT_ALL, NULL);
  }

  /* --- SECTION 3: MQTT Broker & Home Assistant Auto-Discovery --- */
  lv_obj_t *mqtt_box = lv_obj_create(panel);
  lv_obj_set_size(mqtt_box, lv_pct(100), LV_SIZE_CONTENT);
  lv_obj_set_style_pad_all(mqtt_box, 10, 0);
  lv_obj_set_style_bg_color(mqtt_box, lv_color_hex(0x161D2B), 0);
  lv_obj_set_style_border_color(mqtt_box, lv_color_hex(0x1F2836), 0);
  lv_obj_set_style_border_width(mqtt_box, 1, 0);
  lv_obj_set_style_radius(mqtt_box, 8, 0);
  lv_obj_set_flex_flow(mqtt_box, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_row(mqtt_box, 6, 0);

  lv_obj_t *m_title = lv_label_create(mqtt_box);
  lv_label_set_text(m_title, LV_SYMBOL_UPLOAD " MQTT Broker");
  lv_obj_add_style(m_title, &style_title, 0);

  net_mqtt_status_label = lv_label_create(mqtt_box);
  lv_label_set_text(net_mqtt_status_label, "MQTT Status: Disconnected");
  lv_obj_add_style(net_mqtt_status_label, &style_text_muted, 0);

  lv_obj_t *m_host_lbl = lv_label_create(mqtt_box);
  lv_label_set_text(m_host_lbl, "Broker Server Address");
  lv_obj_add_style(m_host_lbl, &style_text_muted, 0);

  lv_obj_t *m_host_ta = lv_textarea_create(mqtt_box);
  lv_obj_set_width(m_host_ta, lv_pct(100));
  lv_textarea_set_one_line(m_host_ta, true);
  lv_textarea_set_placeholder_text(m_host_ta, "192.168.1.200 or mqtt.home");
  lv_obj_add_event_cb(m_host_ta, net_ta_event_cb, LV_EVENT_ALL, NULL);

  lv_obj_t *m_port_lbl = lv_label_create(mqtt_box);
  lv_label_set_text(m_port_lbl, "Port");
  lv_obj_add_style(m_port_lbl, &style_text_muted, 0);

  lv_obj_t *m_port_ta = lv_textarea_create(mqtt_box);
  lv_obj_set_width(m_port_ta, lv_pct(100));
  lv_textarea_set_one_line(m_port_ta, true);
  lv_textarea_set_placeholder_text(m_port_ta, "1883");
  lv_obj_add_event_cb(m_port_ta, net_ta_event_cb, LV_EVENT_ALL, NULL);

  lv_obj_t *m_user_lbl = lv_label_create(mqtt_box);
  lv_label_set_text(m_user_lbl, "Username / Password");
  lv_obj_add_style(m_user_lbl, &style_text_muted, 0);

  lv_obj_t *m_user_ta = lv_textarea_create(mqtt_box);
  lv_obj_set_width(m_user_ta, lv_pct(100));
  lv_textarea_set_one_line(m_user_ta, true);
  lv_textarea_set_placeholder_text(m_user_ta, "MQTT Username");
  lv_obj_add_event_cb(m_user_ta, net_ta_event_cb, LV_EVENT_ALL, NULL);

  lv_obj_t *m_pass_ta = lv_textarea_create(mqtt_box);
  lv_obj_set_width(m_pass_ta, lv_pct(100));
  lv_textarea_set_one_line(m_pass_ta, true);
  lv_textarea_set_password_mode(m_pass_ta, true);
  lv_textarea_set_placeholder_text(m_pass_ta, "MQTT Password");
  lv_obj_add_event_cb(m_pass_ta, net_ta_event_cb, LV_EVENT_ALL, NULL);

  lv_obj_t *m_conn_btn = lv_btn_create(mqtt_box);
  lv_obj_set_width(m_conn_btn, lv_pct(100));
  lv_obj_set_style_bg_color(m_conn_btn, current_accent_color, 0);
  lv_obj_t *m_conn_lbl = lv_label_create(m_conn_btn);
  lv_label_set_text(m_conn_lbl, LV_SYMBOL_OK " Connect MQTT");
  lv_obj_center(m_conn_lbl);
  lv_obj_add_event_cb(m_conn_btn, mqtt_connect_btn_cb, LV_EVENT_CLICKED, NULL);

  /* Home Assistant Auto-Discovery Section */
  lv_obj_t *ha_disc_btn = lv_btn_create(mqtt_box);
  lv_obj_set_width(ha_disc_btn, lv_pct(100));
  lv_obj_set_style_bg_color(ha_disc_btn, lv_color_hex(0x10B981), 0); /* Emerald Green */
  lv_obj_t *ha_disc_lbl = lv_label_create(ha_disc_btn);
  lv_label_set_text(ha_disc_lbl, LV_SYMBOL_UPLOAD " Publish HA Auto-Discovery");
  lv_obj_center(ha_disc_lbl);
  lv_obj_add_event_cb(ha_disc_btn, ha_discovery_btn_cb, LV_EVENT_CLICKED, NULL);

  net_ha_disc_status_label = lv_label_create(mqtt_box);
  lv_label_set_text(net_ha_disc_status_label, "Home Assistant: Ready to publish");
  lv_obj_add_style(net_ha_disc_status_label, &style_text_muted, 0);
}

/*SD Card tab*/
static void sd_update_mount_button(void) {
  if (!sd_mount_label)
    return;
  if (sd_card_is_mounted())
    lv_label_set_text(sd_mount_label, "Unmount SD");
  else
    lv_label_set_text(sd_mount_label, "Mount SD");
}

static void sd_update_status(const char *msg) {
  if (sd_status_label)
    lv_label_set_text(sd_status_label, msg);
}

static void sd_card_tab_create(lv_obj_t *parent) {
  /*Main container - two columns side by side*/
  lv_obj_t *main_cont = lv_obj_create(parent);
  lv_obj_set_size(main_cont, lv_pct(100), lv_pct(100));
  lv_obj_set_style_pad_all(main_cont, 8, 0);
  lv_obj_set_style_border_width(main_cont, 0, 0);
  lv_obj_set_style_bg_opa(main_cont, LV_OPA_TRANSP, 0);
  lv_obj_set_flex_flow(main_cont, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(main_cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START,
                        LV_FLEX_ALIGN_START);
  lv_obj_clear_flag(main_cont, LV_OBJ_FLAG_SCROLLABLE);

  /*LEFT COLUMN - File Tree (60% width)*/
  lv_obj_t *left_panel = lv_obj_create(main_cont);
  sd_left_panel = left_panel;
  lv_obj_set_size(left_panel, lv_pct(60), lv_pct(100));
  lv_obj_set_style_pad_all(left_panel, 8, 0);
  lv_obj_set_style_bg_color(left_panel, lv_color_hex(0x111622), 0);
  lv_obj_set_style_border_color(left_panel, current_accent_color, 0);
  lv_obj_set_style_border_width(left_panel, 1, 0);
  lv_obj_set_flex_flow(left_panel, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(left_panel, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START,
                        LV_FLEX_ALIGN_START);

  lv_obj_t *left_title = lv_label_create(left_panel);
  lv_label_set_text(left_title, LV_SYMBOL_DIRECTORY " Files");
  lv_obj_add_style(left_title, &style_title, 0);

  sd_list = lv_list_create(left_panel);
  lv_obj_set_width(sd_list, lv_pct(100));
  lv_obj_set_flex_grow(sd_list, 1); /*Take remaining space*/

  /*RIGHT COLUMN - Tools (40% width)*/
  lv_obj_t *right_panel = lv_obj_create(main_cont);
  sd_right_panel = right_panel;
  lv_obj_set_size(right_panel, lv_pct(40), lv_pct(100));
  lv_obj_set_style_pad_all(right_panel, 8, 0);
  lv_obj_set_style_bg_color(right_panel, lv_color_hex(0x111622), 0);
  lv_obj_set_style_border_color(right_panel, current_accent_color, 0);
  lv_obj_set_style_border_width(right_panel, 1, 0);
  lv_obj_set_flex_flow(right_panel, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(right_panel, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START,
                        LV_FLEX_ALIGN_START);
  lv_obj_set_style_pad_row(right_panel, 8, 0);

  lv_obj_t *right_title = lv_label_create(right_panel);
  lv_label_set_text(right_title, LV_SYMBOL_SETTINGS " Tools");
  lv_obj_add_style(right_title, &style_title, 0);

  /*Status label*/
  sd_status_label = lv_label_create(right_panel);
  sd_update_status("Not mounted");

  /*Mount/Unmount button*/
  sd_mount_btn = lv_btn_create(right_panel);
  lv_obj_set_width(sd_mount_btn, lv_pct(100));
  lv_obj_set_style_bg_color(sd_mount_btn, current_accent_color, 0);
  sd_mount_label = lv_label_create(sd_mount_btn);
  lv_label_set_text(sd_mount_label, "Mount SD");
  lv_obj_center(sd_mount_label);
  lv_obj_add_event_cb(sd_mount_btn, sd_mount_btn_event_cb, LV_EVENT_CLICKED,
                      NULL);

  /*Refresh button*/
  lv_obj_t *refresh_btn = lv_btn_create(right_panel);
  lv_obj_set_width(refresh_btn, lv_pct(100));
  lv_obj_t *refresh_label = lv_label_create(refresh_btn);
  lv_label_set_text(refresh_label, LV_SYMBOL_REFRESH " Refresh");
  lv_obj_center(refresh_label);
  lv_obj_add_event_cb(refresh_btn, sd_refresh_btn_event_cb, LV_EVENT_CLICKED,
                      NULL);

  /*Format button*/
  sd_format_btn = lv_btn_create(right_panel);
  lv_obj_set_width(sd_format_btn, lv_pct(100));
  lv_obj_set_style_bg_color(sd_format_btn, lv_palette_main(LV_PALETTE_RED), 0);
  lv_obj_t *fmt_label = lv_label_create(sd_format_btn);
  lv_label_set_text(fmt_label, LV_SYMBOL_TRASH " Format SD");
  lv_obj_center(fmt_label);
  lv_obj_add_event_cb(sd_format_btn, sd_format_btn_event_cb, LV_EVENT_CLICKED,
                      NULL);

  /*Test Log button*/
  lv_obj_t *test_log_btn = lv_btn_create(right_panel);
  lv_obj_set_width(test_log_btn, lv_pct(100));
  lv_obj_set_style_bg_color(test_log_btn, lv_palette_main(LV_PALETTE_GREEN), 0);
  lv_obj_t *test_log_label = lv_label_create(test_log_btn);
  lv_label_set_text(test_log_label, LV_SYMBOL_EDIT " Test Log");
  lv_obj_center(test_log_label);
  lv_obj_add_event_cb(test_log_btn, sd_test_log_btn_event_cb, LV_EVENT_CLICKED,
                      NULL);

  /*Try auto-mount SD card on startup*/
  printf("[SD] Auto-mounting SD card...\n");
  if (!sd_card_is_mounted()) {
    if (sd_card_init()) {
      printf("[SD] Auto-mount success!\n");
      sd_update_status("Auto-mounted");

      /*Initialize logger with SD card*/
      logger_init();
      log_info("AUTOMONT", "SD Card auto-mounted and logger initialized");
    } else {
      printf("[SD] Auto-mount failed - manual mount required\n");
      sd_update_status("Not mounted");
    }
  } else {
    printf("[SD] SD already mounted\n");
    sd_update_status("Mounted");
  }

  sd_update_mount_button();
  sd_refresh_list();
}

#if LV_USE_PERF_MONITOR
static lv_obj_t *perf_monitor_label = NULL;

static lv_obj_t *perf_monitor_find_label(void) {
  /*Locate the built-in LVGL performance monitor label on the system layer*/
  lv_obj_t *sys = lv_layer_sys();
  if (!sys)
    return NULL;
  uint32_t cnt = lv_obj_get_child_cnt(sys);
  for (uint32_t i = 0; i < cnt; i++) {
    lv_obj_t *child = lv_obj_get_child(sys, i);
    if (child && lv_obj_check_type(child, &lv_label_class)) {
      return child;
    }
  }
  return NULL;
}

static void perf_monitor_switch_cb(lv_event_t *e) {
  lv_obj_t *obj = lv_event_get_target(e);

  if (perf_monitor_label == NULL) {
    perf_monitor_label = perf_monitor_find_label();
  }

  if (perf_monitor_label) {
    if (lv_obj_has_state(obj, LV_STATE_CHECKED)) {
      lv_obj_clear_flag(perf_monitor_label, LV_OBJ_FLAG_HIDDEN);
    } else {
      lv_obj_add_flag(perf_monitor_label, LV_OBJ_FLAG_HIDDEN);
    }
  }
}

static void perf_monitor_init_cb(lv_timer_t *t) {
  lv_obj_t *chk = (lv_obj_t *)t->user_data;
  perf_monitor_label = perf_monitor_find_label();
  if (perf_monitor_label) {
    if (chk && lv_obj_has_state(chk, LV_STATE_CHECKED)) {
      lv_obj_clear_flag(perf_monitor_label, LV_OBJ_FLAG_HIDDEN);
    } else {
      lv_obj_add_flag(perf_monitor_label, LV_OBJ_FLAG_HIDDEN);
    }
    lv_timer_del(
        t); /* Stop timer only when label is successfully found and updated */
  }
}
#endif

static void backlight_slider_cb(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t *slider = lv_event_get_target(e);
  int32_t val = lv_slider_get_value(slider);

  /*Update backlight on value change*/
  if (code == LV_EVENT_VALUE_CHANGED) {
    backlight_set((uint8_t)val);
  }

  /*Draw value label above knob like Profile Experience slider*/
  if (code == LV_EVENT_REFR_EXT_DRAW_SIZE) {
    lv_coord_t *s = lv_event_get_param(e);
    *s = LV_MAX(*s, 60);
  } else if (code == LV_EVENT_DRAW_PART_END) {
    lv_obj_draw_part_dsc_t *dsc = lv_event_get_param(e);
    if (dsc->part == LV_PART_KNOB &&
        lv_obj_has_state(slider, LV_STATE_PRESSED)) {
      char buf[8];
      lv_snprintf(buf, sizeof(buf), "%" LV_PRId32 "%%", val);

      lv_point_t text_size;
      lv_txt_get_size(&text_size, buf, font_normal, 0, 0, LV_COORD_MAX,
                      LV_TEXT_FLAG_NONE);

      lv_area_t txt_area;
      txt_area.x1 = dsc->draw_area->x1 + lv_area_get_width(dsc->draw_area) / 2 -
                    text_size.x / 2;
      txt_area.x2 = txt_area.x1 + text_size.x;
      txt_area.y2 = dsc->draw_area->y1 - 10;
      txt_area.y1 = txt_area.y2 - text_size.y;

      lv_area_t bg_area;
      bg_area.x1 = txt_area.x1 - LV_DPX(8);
      bg_area.x2 = txt_area.x2 + LV_DPX(8);
      bg_area.y1 = txt_area.y1 - LV_DPX(8);
      bg_area.y2 = txt_area.y2 + LV_DPX(8);

      lv_draw_rect_dsc_t rect_dsc;
      lv_draw_rect_dsc_init(&rect_dsc);
      rect_dsc.bg_color = lv_palette_darken(LV_PALETTE_GREY, 3);
      rect_dsc.radius = LV_DPX(5);
      lv_draw_rect(dsc->draw_ctx, &rect_dsc, &bg_area);

      lv_draw_label_dsc_t label_dsc;
      lv_draw_label_dsc_init(&label_dsc);
      label_dsc.color = lv_color_white();
      label_dsc.font = font_normal;
      lv_draw_label(dsc->draw_ctx, &label_dsc, &txt_area, buf, NULL);
    }
  }
}

static void reset_btn_event_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED)
    return;
  esp_restart();
}

/*SD list iterator callback to fill LVGL list*/
static void sd_list_iter_cb(const char *name, bool is_dir, uint32_t size,
                            void *ctx) {
  lv_obj_t *list = (lv_obj_t *)ctx;
  if (!list)
    return;

  char label[64];
  if (is_dir) {
    lv_snprintf(label, sizeof(label), "%s/", name);
    lv_list_add_btn(list, LV_SYMBOL_DIRECTORY, label);
  } else {
    lv_snprintf(label, sizeof(label), "%s (%luB)", name, (unsigned long)size);
    lv_list_add_btn(list, LV_SYMBOL_FILE, label);
  }
}

/*Callback for list item click - browse folders*/
static void sd_list_item_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED)
    return;

  lv_obj_t *btn = lv_event_get_target(e);
  if (!btn)
    return;

  /*Find which item was clicked*/
  uint32_t index = lv_obj_get_child_id(btn);
  if (index >= sd_entry_count)
    return;

  sd_entry_t *entry = &sd_entries[index];

  printf("[SD] Clicked: %s (is_dir=%d)\n", entry->name, entry->is_directory);

  if (entry->is_directory) {
    /*Navigate into folder*/
    strncpy(sd_current_path, entry->full_path, sizeof(sd_current_path) - 1);
    sd_current_path[sizeof(sd_current_path) - 1] = '\0';
    printf("[SD] Navigating to: %s\n", sd_current_path);
    sd_refresh_list();
  } else {
    /*Show file info or allow download*/
    char buf[64];
    lv_snprintf(buf, sizeof(buf), "File: %s (%lu bytes)", entry->name,
                (unsigned long)entry->size);
    sd_update_status(buf);
  }
}

static void sd_refresh_list(void) {
  printf("[SD] sd_refresh_list called (path=%s)\n", sd_current_path);
  if (!sd_list) {
    printf("[SD] sd_list is NULL!\n");
    return;
  }
  lv_obj_clean(sd_list);

  if (!sd_card_is_mounted()) {
    printf("[SD] Card not mounted\n");
    sd_update_status("Not mounted");
    sd_update_mount_button();
    return;
  }

  /*Load entries from current directory*/
  sd_entry_count = sd_card_list_dir_browse(sd_current_path, sd_entries, 32);
  printf("[SD] Found %d entries in %s\n", sd_entry_count, sd_current_path);

  /*Debug: print all entries*/
  for (int i = 0; i < sd_entry_count; i++) {
    printf("[SD] Entry %d: %s (dir=%d, size=%lu)\n", i, sd_entries[i].name,
           sd_entries[i].is_directory, (unsigned long)sd_entries[i].size);
  }

  /*Add parent directory button if not at root*/
  if (strcmp(sd_current_path, "/") != 0) {
    lv_obj_t *parent_btn = lv_list_add_btn(sd_list, LV_SYMBOL_UP, "..");
    lv_obj_add_event_cb(parent_btn, sd_list_parent_cb, LV_EVENT_CLICKED, NULL);
  }

  /*Add all entries*/
  for (int i = 0; i < sd_entry_count; i++) {
    char label[64];
    if (sd_entries[i].is_directory) {
      lv_snprintf(label, sizeof(label), "%s/", sd_entries[i].name);
      lv_obj_t *btn = lv_list_add_btn(sd_list, LV_SYMBOL_DIRECTORY, label);
      lv_obj_add_event_cb(btn, sd_list_item_cb, LV_EVENT_CLICKED, NULL);
    } else {
      lv_snprintf(label, sizeof(label), "%s (%lu B)", sd_entries[i].name,
                  (unsigned long)sd_entries[i].size);
      lv_obj_t *btn = lv_list_add_btn(sd_list, LV_SYMBOL_FILE, label);
      lv_obj_add_event_cb(btn, sd_list_item_cb, LV_EVENT_CLICKED, NULL);
    }
  }

  /*Update status*/
  char status[64];
  if (sd_entry_count == 0) {
    lv_snprintf(status, sizeof(status), "Empty folder");
  } else {
    lv_snprintf(status, sizeof(status), "%d items", sd_entry_count);
  }
  sd_update_status(status);
  sd_update_mount_button();
}

/*Callback for parent directory button*/
static void sd_list_parent_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED)
    return;

  /*Go to parent directory*/
  if (strcmp(sd_current_path, "/") == 0) {
    return; /* Already at root */
  }

  /*Find last slash and trim*/
  char *last_slash = strrchr(sd_current_path, '/');
  if (last_slash != sd_current_path) {
    *last_slash = '\0'; /* Trim path */
  } else {
    strcpy(sd_current_path, "/"); /* Go to root */
  }

  printf("[SD] Going back to: %s\n", sd_current_path);
  sd_refresh_list();
}

static void sd_mount_btn_event_cb(lv_event_t *e) {
  printf("[SD] Mount button clicked!\n");
  if (lv_event_get_code(e) != LV_EVENT_CLICKED)
    return;
  printf("[SD] Processing mount/unmount...\n");

  if (sd_card_is_mounted()) {
    printf("[SD] Unmounting...\n");
    sd_card_unmount();
    sd_update_status("Unmounted");
  } else {
    printf("[SD] Mounting...\n");
    if (sd_card_init()) {
      printf("[SD] Mount success!\n");
      sd_update_status("Mounted");

      /*Initialize logger now that SD is mounted*/
      printf("[SD] Initializing logger...\n");
      logger_init();
      log_info("SD_MOUNT", "Logger initialized after SD mount");
    } else {
      printf("[SD] Mount failed!\n");
      sd_update_status("Mount failed");
    }
  }

  sd_update_mount_button();
  sd_refresh_list();
}

/*Refresh button callback*/
static void sd_refresh_btn_event_cb(lv_event_t *e) {
  printf("[SD] Refresh button clicked!\n");
  if (lv_event_get_code(e) != LV_EVENT_CLICKED)
    return;
  printf("[SD] Processing refresh...\n");
  sd_refresh_list();
}

/*Test Log button callback - writes a test log entry*/
static void sd_test_log_btn_event_cb(lv_event_t *e) {
  printf("[SD] Test Log button clicked!\n");
  if (lv_event_get_code(e) != LV_EVENT_CLICKED)
    return;
  printf("[SD] Writing test log...\n");

  static uint32_t test_counter = 0;
  test_counter++;

  log_info("TEST", "Test log entry #%lu from SD Card tab",
           (unsigned long)test_counter);
  log_debug("TEST", "Debug level test message");
  log_warn("TEST", "Warning level test message");

  /*Flush to ensure it's written to file*/
  logger_flush();

  sd_update_status("Log written!");

  /*Refresh file list to show new/updated log file*/
  sd_refresh_list();
}

/*Format confirmation msgbox callback*/
static void sd_format_confirm_cb(lv_event_t *e) {
  lv_obj_t *mbox = lv_event_get_current_target(e);
  const char *btn_txt = lv_msgbox_get_active_btn_text(mbox);

  if (btn_txt && strcmp(btn_txt, "Format") == 0) {
    sd_update_status("Formatting...");
    lv_refr_now(NULL); /*Force refresh to show status*/

    if (sd_card_format()) {
      sd_update_status("Format OK");
    } else {
      sd_update_status("Format failed");
    }
    sd_refresh_list();
  }

  lv_msgbox_close(mbox);
}

static void sd_format_btn_event_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED)
    return;

  if (!sd_card_is_mounted()) {
    sd_update_status("Mount first");
    return;
  }

  /*Show confirmation dialog*/
  static const char *btns[] = {"Format", "Cancel", ""};
  lv_obj_t *mbox =
      lv_msgbox_create(NULL, "Format SD Card",
                       "All data will be deleted!\nAre you sure?", btns, false);
  lv_obj_add_event_cb(mbox, sd_format_confirm_cb, LV_EVENT_VALUE_CHANGED, NULL);
  lv_obj_center(mbox);
}

static void profile_create(lv_obj_t *parent) {
  lv_obj_set_style_pad_top(parent, 10, 0);
  lv_obj_set_style_pad_hor(parent, 10, 0);
  lv_obj_set_style_pad_bottom(parent, 6, 0);

  lv_obj_t *panel1 = lv_obj_create(parent);
  profile_panel1 = panel1;
  lv_obj_set_height(panel1, 192);
  lv_obj_set_style_pad_ver(panel1, 6, 0);
  lv_obj_set_style_pad_hor(panel1, 8, 0);
  lv_obj_set_style_bg_color(panel1, lv_color_hex(0x111622), 0);
  lv_obj_set_style_border_color(panel1, current_accent_color, 0);
  lv_obj_set_style_border_width(panel1, 1, 0);

  lv_obj_t *avatar = lv_arc_create(panel1);
  profile_avatar = avatar;
  lv_obj_set_size(avatar, 170, 170);
  lv_arc_set_rotation(avatar, 135);
  lv_arc_set_bg_angles(avatar, 0, 270);
  lv_arc_set_value(avatar, 70);
  lv_obj_set_style_arc_color(avatar, current_accent_color, LV_PART_INDICATOR);
  lv_obj_set_style_arc_color(avatar, lv_color_hex(0x1F2836), LV_PART_MAIN);
  lv_obj_remove_style(avatar, NULL, LV_PART_KNOB);
  lv_obj_clear_flag(avatar, LV_OBJ_FLAG_CLICKABLE);

  /* SOC + Battery icon combined inside the Arc (raised 10px to y=-42) */
  lv_obj_t *bat_soc_lbl = lv_label_create(avatar);
  lv_label_set_text(bat_soc_lbl, "SOC " LV_SYMBOL_BATTERY_3);
  lv_obj_add_style(bat_soc_lbl, &style_text_muted, 0);
  lv_obj_align(bat_soc_lbl, LV_ALIGN_CENTER, 0, -42);

  /* Percentage label inside the Arc (using precompiled Montserrat font) */
  lv_obj_t *pct_lbl = lv_label_create(avatar);
  profile_pct_label = pct_lbl;
  lv_label_set_text(pct_lbl, "70%");
  lv_obj_set_style_text_font(pct_lbl, &lv_font_montserrat_36, 0);
  lv_obj_set_style_text_color(pct_lbl, current_accent_color, 0);
  lv_obj_align(pct_lbl, LV_ALIGN_CENTER, 0, -3);

  /* Glowing Green Status Indicator + Technical Status Text below 70% */
  lv_obj_t *status_row = lv_obj_create(avatar);
  lv_obj_remove_style_all(status_row);
  lv_obj_set_size(status_row, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
  lv_obj_set_flex_flow(status_row, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(status_row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_column(status_row, 6, 0);
  lv_obj_align(status_row, LV_ALIGN_CENTER, 0, 36);

  /* Green Dot (No shadow) */
  lv_obj_t *status_dot = lv_obj_create(status_row);
  lv_obj_remove_style_all(status_dot);
  lv_obj_set_size(status_dot, 8, 8);
  lv_obj_set_style_radius(status_dot, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_color(status_dot, lv_color_hex(0x10B981), 0);
  lv_obj_set_style_bg_opa(status_dot, LV_OPA_COVER, 0);
  lv_obj_set_style_shadow_width(status_dot, 0, 0);

  /* Technical Status Label (Muted Grey like SOC) */
  lv_obj_t *status_txt = lv_label_create(status_row);
  lv_label_set_text(status_txt, "NORMAL");
  lv_obj_add_style(status_txt, &style_text_muted, 0);

  /* Create 6 sub-containers in 2 columns x 3 rows on the right */
  lv_obj_t *boxes_cont = lv_obj_create(panel1);
  lv_obj_set_height(boxes_cont, 170);
  lv_obj_set_style_pad_all(boxes_cont, 2, 0);
  lv_obj_set_style_bg_opa(boxes_cont, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(boxes_cont, 0, 0);
  lv_obj_clear_flag(boxes_cont, LV_OBJ_FLAG_SCROLLABLE);

  static lv_coord_t sub_col_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1),
                                     LV_GRID_TEMPLATE_LAST};
  static lv_coord_t sub_row_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1),
                                     LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
  lv_obj_set_grid_dsc_array(boxes_cont, sub_col_dsc, sub_row_dsc);

  static const char *box_titles[6] = {"Voltage",     "SOC",   "Current",
                                      "Temperature", "Power", "Status"};
  static const char *box_values[6] = {"320V",   "70%",   "1.8A",
                                      "24.5°C", "202.3W", "OK"};

  for (int i = 0; i < 6; i++) {
    lv_obj_t *box = lv_obj_create(boxes_cont);
    profile_sub_boxes[i] = box;
    lv_obj_set_style_pad_all(box, 3, 0);
    lv_obj_set_style_bg_color(box, lv_color_hex(0x111622), 0);
    lv_obj_set_style_border_color(box, current_accent_color, 0);
    lv_obj_set_style_border_width(box, 1, 0);
    lv_obj_clear_flag(box, LV_OBJ_FLAG_SCROLLABLE);

    int col = i % 2;
    int row = i / 2;
    lv_obj_set_grid_cell(box, LV_GRID_ALIGN_STRETCH, col, 1,
                         LV_GRID_ALIGN_STRETCH, row, 1);

    lv_obj_t *title_lbl = lv_label_create(box);
    lv_label_set_text(title_lbl, box_titles[i]);
    lv_obj_add_style(title_lbl, &style_text_muted, 0);
    lv_obj_align(title_lbl, LV_ALIGN_TOP_LEFT, 2, 2);

    lv_obj_t *val_lbl = lv_label_create(box);
    lv_label_set_text(val_lbl, box_values[i]);
    lv_obj_add_style(val_lbl, &style_title, 0);
    lv_obj_align(val_lbl, LV_ALIGN_BOTTOM_LEFT, 2, -2);
    profile_sub_box_val_labels[i] = val_lbl;
  }

  if (disp_size == DISP_LARGE || disp_size == DISP_MEDIUM) {
    static lv_coord_t grid_main_col_dsc[] = {LV_GRID_FR(1),
                                             LV_GRID_TEMPLATE_LAST};
    static lv_coord_t grid_main_row_dsc[] = {LV_GRID_CONTENT,
                                             LV_GRID_TEMPLATE_LAST};
    static lv_coord_t grid_1_col_dsc[] = {LV_GRID_CONTENT, 10, LV_GRID_FR(1),
                                          LV_GRID_TEMPLATE_LAST};
    static lv_coord_t grid_1_row_dsc[] = {LV_GRID_CONTENT,
                                          LV_GRID_TEMPLATE_LAST};

    lv_obj_set_grid_dsc_array(parent, grid_main_col_dsc, grid_main_row_dsc);
    lv_obj_set_grid_cell(panel1, LV_GRID_ALIGN_STRETCH, 0, 1,
                         LV_GRID_ALIGN_CENTER, 0, 1);

    lv_obj_set_grid_dsc_array(panel1, grid_1_col_dsc, grid_1_row_dsc);
    lv_obj_set_grid_cell(avatar, LV_GRID_ALIGN_CENTER, 0, 1,
                         LV_GRID_ALIGN_CENTER, 0, 1);
    lv_obj_set_grid_cell(boxes_cont, LV_GRID_ALIGN_STRETCH, 2, 1,
                         LV_GRID_ALIGN_STRETCH, 0, 1);
  } else if (disp_size == DISP_SMALL) {
    static lv_coord_t grid_main_col_dsc[] = {LV_GRID_FR(1),
                                             LV_GRID_TEMPLATE_LAST};
    static lv_coord_t grid_main_row_dsc[] = {LV_GRID_CONTENT,
                                             LV_GRID_TEMPLATE_LAST};
    static lv_coord_t grid_1_col_dsc[] = {LV_GRID_CONTENT, LV_GRID_FR(1),
                                          LV_GRID_TEMPLATE_LAST};
    static lv_coord_t grid_1_row_dsc[] = {LV_GRID_CONTENT,
                                          LV_GRID_TEMPLATE_LAST};

    lv_obj_set_grid_dsc_array(parent, grid_main_col_dsc, grid_main_row_dsc);
    lv_obj_set_grid_cell(panel1, LV_GRID_ALIGN_STRETCH, 0, 1,
                         LV_GRID_ALIGN_CENTER, 0, 1);

    lv_obj_set_grid_dsc_array(panel1, grid_1_col_dsc, grid_1_row_dsc);
    lv_obj_set_grid_cell(avatar, LV_GRID_ALIGN_CENTER, 0, 1,
                         LV_GRID_ALIGN_CENTER, 0, 1);
    lv_obj_set_grid_cell(boxes_cont, LV_GRID_ALIGN_STRETCH, 1, 1,
                         LV_GRID_ALIGN_STRETCH, 0, 1);
  }

  if (!profile_soc_timer) {
    profile_soc_timer = lv_timer_create(profile_soc_timer_cb, 160, NULL);
  }
}

static void analytics_create(lv_obj_t *parent) {
  lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_ROW_WRAP);

  static lv_coord_t grid_chart_row_dsc[] = {LV_GRID_CONTENT, LV_GRID_FR(1), 10,
                                            LV_GRID_TEMPLATE_LAST};
  static lv_coord_t grid_chart_col_dsc[] = {20, LV_GRID_FR(1),
                                            LV_GRID_TEMPLATE_LAST};

  lv_obj_t *chart1_cont = lv_obj_create(parent);
  lv_obj_set_flex_grow(chart1_cont, 1);
  lv_obj_set_grid_dsc_array(chart1_cont, grid_chart_col_dsc,
                            grid_chart_row_dsc);

  lv_obj_set_height(chart1_cont, LV_PCT(100));
  lv_obj_set_style_max_height(chart1_cont, 300, 0);

  lv_obj_t *title = lv_label_create(chart1_cont);
  lv_label_set_text(title, "Unique visitors");
  lv_obj_add_style(title, &style_title, 0);
  lv_obj_set_grid_cell(title, LV_GRID_ALIGN_START, 0, 2, LV_GRID_ALIGN_START, 0,
                       1);

  chart1 = lv_chart_create(chart1_cont);
  lv_group_add_obj(lv_group_get_default(), chart1);
  lv_obj_add_flag(chart1, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
  lv_obj_set_grid_cell(chart1, LV_GRID_ALIGN_STRETCH, 1, 1,
                       LV_GRID_ALIGN_STRETCH, 1, 1);
  lv_chart_set_axis_tick(chart1, LV_CHART_AXIS_PRIMARY_Y, 0, 0, 5, 1, true, 80);
  lv_chart_set_axis_tick(chart1, LV_CHART_AXIS_PRIMARY_X, 0, 0, 12, 1, true,
                         50);
  lv_chart_set_div_line_count(chart1, 0, 12);
  lv_chart_set_point_count(chart1, 12);
  lv_obj_add_event_cb(chart1, chart_event_cb, LV_EVENT_ALL, NULL);
  if (disp_size == DISP_SMALL)
    lv_chart_set_zoom_x(chart1, 256 * 3);
  else if (disp_size == DISP_MEDIUM)
    lv_chart_set_zoom_x(chart1, 256 * 2);

  lv_obj_set_style_border_side(chart1,
                               LV_BORDER_SIDE_LEFT | LV_BORDER_SIDE_BOTTOM, 0);
  lv_obj_set_style_radius(chart1, 0, 0);

  ser1 = lv_chart_add_series(chart1, lv_theme_get_color_primary(chart1),
                             LV_CHART_AXIS_PRIMARY_Y);
  lv_chart_set_next_value(chart1, ser1, lv_rand(10, 80));
  lv_chart_set_next_value(chart1, ser1, lv_rand(10, 80));
  lv_chart_set_next_value(chart1, ser1, lv_rand(10, 80));
  lv_chart_set_next_value(chart1, ser1, lv_rand(10, 80));
  lv_chart_set_next_value(chart1, ser1, lv_rand(10, 80));
  lv_chart_set_next_value(chart1, ser1, lv_rand(10, 80));
  lv_chart_set_next_value(chart1, ser1, lv_rand(10, 80));
  lv_chart_set_next_value(chart1, ser1, lv_rand(10, 80));
  lv_chart_set_next_value(chart1, ser1, lv_rand(10, 80));
  lv_chart_set_next_value(chart1, ser1, lv_rand(10, 80));
  lv_chart_set_next_value(chart1, ser1, lv_rand(10, 80));
  lv_chart_set_next_value(chart1, ser1, lv_rand(10, 80));
  lv_chart_set_next_value(chart1, ser1, lv_rand(10, 80));

  lv_obj_t *chart2_cont = lv_obj_create(parent);
  lv_obj_add_flag(chart2_cont, LV_OBJ_FLAG_FLEX_IN_NEW_TRACK);
  lv_obj_set_flex_grow(chart2_cont, 1);

  lv_obj_set_height(chart2_cont, LV_PCT(100));
  lv_obj_set_style_max_height(chart2_cont, 300, 0);

  lv_obj_set_grid_dsc_array(chart2_cont, grid_chart_col_dsc,
                            grid_chart_row_dsc);

  title = lv_label_create(chart2_cont);
  lv_label_set_text(title, "Monthly revenue");
  lv_obj_add_style(title, &style_title, 0);
  lv_obj_set_grid_cell(title, LV_GRID_ALIGN_START, 0, 2, LV_GRID_ALIGN_START, 0,
                       1);

  chart2 = lv_chart_create(chart2_cont);
  lv_group_add_obj(lv_group_get_default(), chart2);
  lv_obj_add_flag(chart2, LV_OBJ_FLAG_SCROLL_ON_FOCUS);

  lv_obj_set_grid_cell(chart2, LV_GRID_ALIGN_STRETCH, 1, 1,
                       LV_GRID_ALIGN_STRETCH, 1, 1);
  lv_chart_set_axis_tick(chart2, LV_CHART_AXIS_PRIMARY_Y, 0, 0, 5, 1, true, 80);
  lv_chart_set_axis_tick(chart2, LV_CHART_AXIS_PRIMARY_X, 0, 0, 12, 1, true,
                         50);
  lv_obj_set_size(chart2, LV_PCT(100), LV_PCT(100));
  lv_chart_set_type(chart2, LV_CHART_TYPE_BAR);
  lv_chart_set_div_line_count(chart2, 6, 0);
  lv_chart_set_point_count(chart2, 12);
  lv_obj_add_event_cb(chart2, chart_event_cb, LV_EVENT_ALL, NULL);
  lv_chart_set_zoom_x(chart2, 256 * 2);
  lv_obj_set_style_border_side(chart2,
                               LV_BORDER_SIDE_LEFT | LV_BORDER_SIDE_BOTTOM, 0);
  lv_obj_set_style_radius(chart2, 0, 0);

  if (disp_size == DISP_SMALL) {
    lv_obj_set_style_pad_gap(chart2, 0, LV_PART_ITEMS);
    lv_obj_set_style_pad_gap(chart2, 2, LV_PART_MAIN);
  } else if (disp_size == DISP_LARGE) {
    lv_obj_set_style_pad_gap(chart2, 16, 0);
  }

  ser2 = lv_chart_add_series(chart2, lv_palette_lighten(LV_PALETTE_GREY, 1),
                             LV_CHART_AXIS_PRIMARY_Y);
  lv_chart_set_next_value(chart2, ser2, lv_rand(10, 80));
  lv_chart_set_next_value(chart2, ser2, lv_rand(10, 80));
  lv_chart_set_next_value(chart2, ser2, lv_rand(10, 80));
  lv_chart_set_next_value(chart2, ser2, lv_rand(10, 80));
  lv_chart_set_next_value(chart2, ser2, lv_rand(10, 80));
  lv_chart_set_next_value(chart2, ser2, lv_rand(10, 80));
  lv_chart_set_next_value(chart2, ser2, lv_rand(10, 80));
  lv_chart_set_next_value(chart2, ser2, lv_rand(10, 80));
  lv_chart_set_next_value(chart2, ser2, lv_rand(10, 80));
  lv_chart_set_next_value(chart2, ser2, lv_rand(10, 80));
  lv_chart_set_next_value(chart2, ser2, lv_rand(10, 80));
  lv_chart_set_next_value(chart2, ser2, lv_rand(10, 80));
  lv_chart_set_next_value(chart2, ser2, lv_rand(10, 80));

  ser3 = lv_chart_add_series(chart2, lv_theme_get_color_primary(chart1),
                             LV_CHART_AXIS_PRIMARY_Y);
  lv_chart_set_next_value(chart2, ser3, lv_rand(10, 80));
  lv_chart_set_next_value(chart2, ser3, lv_rand(10, 80));
  lv_chart_set_next_value(chart2, ser3, lv_rand(10, 80));
  lv_chart_set_next_value(chart2, ser3, lv_rand(10, 80));
  lv_chart_set_next_value(chart2, ser3, lv_rand(10, 80));
  lv_chart_set_next_value(chart2, ser3, lv_rand(10, 80));
  lv_chart_set_next_value(chart2, ser3, lv_rand(10, 80));
  lv_chart_set_next_value(chart2, ser3, lv_rand(10, 80));
  lv_chart_set_next_value(chart2, ser3, lv_rand(10, 80));
  lv_chart_set_next_value(chart2, ser3, lv_rand(10, 80));
  lv_chart_set_next_value(chart2, ser3, lv_rand(10, 80));
  lv_chart_set_next_value(chart2, ser3, lv_rand(10, 80));
  lv_chart_set_next_value(chart2, ser3, lv_rand(10, 80));

  lv_meter_scale_t *scale;
  lv_meter_indicator_t *indic;
  meter1 = create_meter_box(parent, "Monthly Target", "Revenue: 63%",
                            "Sales: 44%", "Costs: 58%");
  lv_obj_add_flag(lv_obj_get_parent(meter1), LV_OBJ_FLAG_FLEX_IN_NEW_TRACK);
  scale = lv_meter_add_scale(meter1);
  lv_meter_set_scale_range(meter1, scale, 0, 100, 270, 90);
  lv_meter_set_scale_ticks(meter1, scale, 0, 0, 0, lv_color_black());

  lv_anim_t a;
  lv_anim_init(&a);
  lv_anim_set_values(&a, 20, 100);
  lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);

  indic =
      lv_meter_add_arc(meter1, scale, 15, lv_palette_main(LV_PALETTE_BLUE), 0);
  lv_anim_set_exec_cb(&a, meter1_indic1_anim_cb);
  lv_anim_set_var(&a, indic);
  lv_anim_set_time(&a, 4100);
  lv_anim_set_playback_time(&a, 2700);
  lv_anim_start(&a);

  indic =
      lv_meter_add_arc(meter1, scale, 15, lv_palette_main(LV_PALETTE_RED), -20);
  lv_anim_set_exec_cb(&a, meter1_indic2_anim_cb);
  lv_anim_set_var(&a, indic);
  lv_anim_set_time(&a, 2600);
  lv_anim_set_playback_time(&a, 3200);
  a.user_data = indic;
  lv_anim_start(&a);

  indic = lv_meter_add_arc(meter1, scale, 15, lv_palette_main(LV_PALETTE_GREEN),
                           -40);
  lv_anim_set_exec_cb(&a, meter1_indic3_anim_cb);
  lv_anim_set_var(&a, indic);
  lv_anim_set_time(&a, 2800);
  lv_anim_set_playback_time(&a, 1800);
  lv_anim_start(&a);

  meter2 =
      create_meter_box(parent, "Sessions", "Desktop: ", "Tablet: ", "Mobile: ");
  if (disp_size < DISP_LARGE)
    lv_obj_add_flag(lv_obj_get_parent(meter2), LV_OBJ_FLAG_FLEX_IN_NEW_TRACK);
  scale = lv_meter_add_scale(meter2);
  lv_meter_set_scale_range(meter2, scale, 0, 100, 360, 90);
  lv_meter_set_scale_ticks(meter2, scale, 0, 0, 0, lv_color_black());

  static lv_meter_indicator_t *meter2_indic[3];
  meter2_indic[0] =
      lv_meter_add_arc(meter2, scale, 20, lv_palette_main(LV_PALETTE_RED), -10);
  lv_meter_set_indicator_start_value(meter2, meter2_indic[0], 0);
  lv_meter_set_indicator_end_value(meter2, meter2_indic[0], 39);

  meter2_indic[1] =
      lv_meter_add_arc(meter2, scale, 30, lv_palette_main(LV_PALETTE_BLUE), 0);
  lv_meter_set_indicator_start_value(meter2, meter2_indic[1], 40);
  lv_meter_set_indicator_end_value(meter2, meter2_indic[1], 69);

  meter2_indic[2] = lv_meter_add_arc(meter2, scale, 10,
                                     lv_palette_main(LV_PALETTE_GREEN), -20);
  lv_meter_set_indicator_start_value(meter2, meter2_indic[2], 70);
  lv_meter_set_indicator_end_value(meter2, meter2_indic[2], 99);

  lv_timer_create(meter2_timer_cb, 100, meter2_indic);

  meter3 = create_meter_box(parent, "Network Speed", "Low speed",
                            "Normal Speed", "High Speed");
  if (disp_size < DISP_LARGE)
    lv_obj_add_flag(lv_obj_get_parent(meter3), LV_OBJ_FLAG_FLEX_IN_NEW_TRACK);

  /*Add a special circle to the needle's pivot*/
  lv_obj_set_style_pad_hor(meter3, 10, 0);
  lv_obj_set_style_size(meter3, 10, LV_PART_INDICATOR);
  lv_obj_set_style_radius(meter3, LV_RADIUS_CIRCLE, LV_PART_INDICATOR);
  lv_obj_set_style_bg_opa(meter3, LV_OPA_COVER, LV_PART_INDICATOR);
  lv_obj_set_style_bg_color(meter3, lv_palette_darken(LV_PALETTE_GREY, 4),
                            LV_PART_INDICATOR);
  lv_obj_set_style_outline_color(meter3, lv_color_white(), LV_PART_INDICATOR);
  lv_obj_set_style_outline_width(meter3, 3, LV_PART_INDICATOR);
  lv_obj_set_style_text_color(meter3, lv_palette_darken(LV_PALETTE_GREY, 1),
                              LV_PART_TICKS);

  scale = lv_meter_add_scale(meter3);
  lv_meter_set_scale_range(meter3, scale, 10, 60, 220, 360 - 220);
  lv_meter_set_scale_ticks(meter3, scale, 21, 3, 17, lv_color_white());
  lv_meter_set_scale_major_ticks(meter3, scale, 4, 4, 22, lv_color_white(), 15);

  indic =
      lv_meter_add_arc(meter3, scale, 10, lv_palette_main(LV_PALETTE_RED), 0);
  lv_meter_set_indicator_start_value(meter3, indic, 0);
  lv_meter_set_indicator_end_value(meter3, indic, 20);

  indic = lv_meter_add_scale_lines(
      meter3, scale, lv_palette_darken(LV_PALETTE_RED, 3),
      lv_palette_darken(LV_PALETTE_RED, 3), true, 0);
  lv_meter_set_indicator_start_value(meter3, indic, 0);
  lv_meter_set_indicator_end_value(meter3, indic, 20);

  indic =
      lv_meter_add_arc(meter3, scale, 12, lv_palette_main(LV_PALETTE_BLUE), 0);
  lv_meter_set_indicator_start_value(meter3, indic, 20);
  lv_meter_set_indicator_end_value(meter3, indic, 40);

  indic = lv_meter_add_scale_lines(
      meter3, scale, lv_palette_darken(LV_PALETTE_BLUE, 3),
      lv_palette_darken(LV_PALETTE_BLUE, 3), true, 0);
  lv_meter_set_indicator_start_value(meter3, indic, 20);
  lv_meter_set_indicator_end_value(meter3, indic, 40);

  indic =
      lv_meter_add_arc(meter3, scale, 10, lv_palette_main(LV_PALETTE_GREEN), 0);
  lv_meter_set_indicator_start_value(meter3, indic, 40);
  lv_meter_set_indicator_end_value(meter3, indic, 60);

  indic = lv_meter_add_scale_lines(
      meter3, scale, lv_palette_darken(LV_PALETTE_GREEN, 3),
      lv_palette_darken(LV_PALETTE_GREEN, 3), true, 0);
  lv_meter_set_indicator_start_value(meter3, indic, 40);
  lv_meter_set_indicator_end_value(meter3, indic, 60);

  indic = lv_meter_add_needle_line(meter3, scale, 4,
                                   lv_palette_darken(LV_PALETTE_GREY, 4), -25);

  lv_obj_t *mbps_label = lv_label_create(meter3);
  lv_label_set_text(mbps_label, "-");
  lv_obj_add_style(mbps_label, &style_title, 0);

  lv_obj_t *mbps_unit_label = lv_label_create(meter3);
  lv_label_set_text(mbps_unit_label, "Mbps");

  lv_anim_init(&a);
  lv_anim_set_values(&a, 10, 60);
  lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
  lv_anim_set_exec_cb(&a, meter3_anim_cb);
  lv_anim_set_var(&a, indic);
  lv_anim_set_time(&a, 4100);
  lv_anim_set_playback_time(&a, 800);
  lv_anim_start(&a);

  lv_obj_update_layout(parent);
  if (disp_size == DISP_MEDIUM) {
    lv_obj_set_size(meter1, 200, 200);
    lv_obj_set_size(meter2, 200, 200);
    lv_obj_set_size(meter3, 200, 200);
  } else {
    lv_coord_t meter_w = lv_obj_get_width(meter1);
    lv_obj_set_height(meter1, meter_w);
    lv_obj_set_height(meter2, meter_w);
    lv_obj_set_height(meter3, meter_w);
  }

  lv_obj_align(mbps_label, LV_ALIGN_TOP_MID, 10, lv_pct(55));
  lv_obj_align_to(mbps_unit_label, mbps_label, LV_ALIGN_OUT_RIGHT_BOTTOM, 10,
                  0);
}

void shop_create(lv_obj_t *parent) {
  lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_ROW_WRAP);

  lv_obj_t *panel1 = lv_obj_create(parent);
  lv_obj_set_size(panel1, lv_pct(100), LV_SIZE_CONTENT);
  lv_obj_set_style_pad_bottom(panel1, 30, 0);

  lv_obj_t *title = lv_label_create(panel1);
  lv_label_set_text(title, "Monthly Summary");
  lv_obj_add_style(title, &style_title, 0);

  lv_obj_t *date = lv_label_create(panel1);
  lv_label_set_text(date, "8-15 July, 2021");
  lv_obj_add_style(date, &style_text_muted, 0);

  lv_obj_t *amount = lv_label_create(panel1);
  lv_label_set_text(amount, "$27,123.25");
  lv_obj_add_style(amount, &style_title, 0);

  lv_obj_t *hint = lv_label_create(panel1);
  lv_label_set_text(hint, LV_SYMBOL_UP " 17% growth this week");
  lv_obj_set_style_text_color(hint, lv_palette_main(LV_PALETTE_GREEN), 0);

  chart3 = lv_chart_create(panel1);
  lv_chart_set_axis_tick(chart3, LV_CHART_AXIS_PRIMARY_Y, 0, 0, 6, 1, true, 80);
  lv_chart_set_axis_tick(chart3, LV_CHART_AXIS_PRIMARY_X, 0, 0, 7, 1, true, 50);
  lv_chart_set_type(chart3, LV_CHART_TYPE_BAR);
  lv_chart_set_div_line_count(chart3, 6, 0);
  lv_chart_set_point_count(chart3, 7);
  lv_obj_add_event_cb(chart3, shop_chart_event_cb, LV_EVENT_ALL, NULL);

  ser4 = lv_chart_add_series(chart3, lv_theme_get_color_primary(chart3),
                             LV_CHART_AXIS_PRIMARY_Y);
  lv_chart_set_next_value(chart3, ser4, lv_rand(60, 90));
  lv_chart_set_next_value(chart3, ser4, lv_rand(60, 90));
  lv_chart_set_next_value(chart3, ser4, lv_rand(60, 90));
  lv_chart_set_next_value(chart3, ser4, lv_rand(60, 90));
  lv_chart_set_next_value(chart3, ser4, lv_rand(60, 90));
  lv_chart_set_next_value(chart3, ser4, lv_rand(60, 90));
  lv_chart_set_next_value(chart3, ser4, lv_rand(60, 90));
  lv_chart_set_next_value(chart3, ser4, lv_rand(60, 90));
  lv_chart_set_next_value(chart3, ser4, lv_rand(60, 90));
  lv_chart_set_next_value(chart3, ser4, lv_rand(60, 90));
  lv_chart_set_next_value(chart3, ser4, lv_rand(60, 90));
  lv_chart_set_next_value(chart3, ser4, lv_rand(60, 90));

  if (disp_size == DISP_LARGE) {
    static lv_coord_t grid1_col_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1),
                                         LV_GRID_TEMPLATE_LAST};
    static lv_coord_t grid1_row_dsc[] = {LV_GRID_CONTENT, /*Title*/
                                         LV_GRID_CONTENT, /*Sub title*/
                                         20,              /*Spacer*/
                                         LV_GRID_CONTENT, /*Amount*/
                                         LV_GRID_CONTENT, /*Hint*/
                                         LV_GRID_TEMPLATE_LAST};

    lv_obj_set_size(chart3, lv_pct(100), lv_pct(100));
    lv_obj_set_style_pad_column(chart3, LV_DPX(30), 0);

    lv_obj_set_grid_dsc_array(panel1, grid1_col_dsc, grid1_row_dsc);
    lv_obj_set_grid_cell(title, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_START,
                         0, 1);
    lv_obj_set_grid_cell(date, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_START,
                         1, 1);
    lv_obj_set_grid_cell(amount, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_START,
                         3, 1);
    lv_obj_set_grid_cell(hint, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_START,
                         4, 1);
    lv_obj_set_grid_cell(chart3, LV_GRID_ALIGN_STRETCH, 1, 1,
                         LV_GRID_ALIGN_STRETCH, 0, 5);
  } else if (disp_size == DISP_MEDIUM) {
    static lv_coord_t grid1_col_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1),
                                         LV_GRID_TEMPLATE_LAST};
    static lv_coord_t grid1_row_dsc[] = {LV_GRID_CONTENT, /*Title + Date*/
                                         LV_GRID_CONTENT, /*Amount + Hint*/
                                         200,             /*Chart*/
                                         LV_GRID_TEMPLATE_LAST};

    lv_obj_update_layout(panel1);
    lv_obj_set_width(chart3, lv_obj_get_content_width(panel1) - 20);
    lv_obj_set_style_pad_column(chart3, LV_DPX(30), 0);

    lv_obj_set_grid_dsc_array(panel1, grid1_col_dsc, grid1_row_dsc);
    lv_obj_set_grid_cell(title, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_CENTER,
                         0, 1);
    lv_obj_set_grid_cell(date, LV_GRID_ALIGN_START, 1, 1, LV_GRID_ALIGN_CENTER,
                         0, 1);
    lv_obj_set_grid_cell(amount, LV_GRID_ALIGN_START, 0, 1,
                         LV_GRID_ALIGN_CENTER, 1, 1);
    lv_obj_set_grid_cell(hint, LV_GRID_ALIGN_START, 1, 1, LV_GRID_ALIGN_CENTER,
                         1, 1);
    lv_obj_set_grid_cell(chart3, LV_GRID_ALIGN_END, 0, 2, LV_GRID_ALIGN_STRETCH,
                         2, 1);
  } else if (disp_size == DISP_SMALL) {
    static lv_coord_t grid1_col_dsc[] = {LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    static lv_coord_t grid1_row_dsc[] = {LV_GRID_CONTENT, /*Title*/
                                         LV_GRID_CONTENT, /*Date*/
                                         LV_GRID_CONTENT, /*Amount*/
                                         LV_GRID_CONTENT, /*Hint*/
                                         LV_GRID_CONTENT, /*Chart*/
                                         LV_GRID_TEMPLATE_LAST};

    lv_obj_set_width(chart3, LV_PCT(95));
    lv_obj_set_height(chart3, LV_VER_RES - 70);
    lv_obj_set_style_max_height(chart3, 300, 0);
    lv_chart_set_zoom_x(chart3, 512);

    lv_obj_set_grid_dsc_array(panel1, grid1_col_dsc, grid1_row_dsc);
    lv_obj_set_grid_cell(title, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_START,
                         0, 1);
    lv_obj_set_grid_cell(date, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_START,
                         1, 1);
    lv_obj_set_grid_cell(amount, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_START,
                         2, 1);
    lv_obj_set_grid_cell(hint, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_START,
                         3, 1);
    lv_obj_set_grid_cell(chart3, LV_GRID_ALIGN_END, 0, 1, LV_GRID_ALIGN_START,
                         4, 1);
  }
}
static void update_accent_color(lv_color_t color) {
  current_accent_color = color;

  /* Icon style */
  lv_style_set_text_color(&style_icon, color);

  /* Profile tab elements */
  if (profile_panel1) {
    lv_obj_set_style_border_color(profile_panel1, color, 0);
  }
  if (profile_avatar) {
    lv_color_t c = (profile_soc_val < 20) ? lv_color_hex(0xDC2626) : color;
    lv_obj_set_style_arc_color(profile_avatar, c, LV_PART_INDICATOR);
  }
  if (profile_pct_label) {
    lv_color_t c = (profile_soc_val < 20) ? lv_color_hex(0xDC2626) : color;
    lv_obj_set_style_text_color(profile_pct_label, c, 0);
  }
  for (int i = 0; i < 6; i++) {
    if (profile_sub_boxes[i]) {
      lv_obj_set_style_border_color(profile_sub_boxes[i], color, 0);
    }
  }

  /* Settings tab elements */
  if (settings_panel) {
    lv_obj_set_style_border_color(settings_panel, color, 0);
  }
  if (network_panel) {
    lv_obj_set_style_border_color(network_panel, color, 0);
  }
  if (reset_btn) {
    lv_obj_set_style_bg_color(reset_btn, color, 0);
  }

  /* SD Card tab elements */
  if (sd_left_panel) {
    lv_obj_set_style_border_color(sd_left_panel, color, 0);
  }
  if (sd_right_panel) {
    lv_obj_set_style_border_color(sd_right_panel, color, 0);
  }
  if (sd_mount_btn) {
    lv_obj_set_style_bg_color(sd_mount_btn, color, 0);
  }

  /* Bottom nav bar active tab border */
  if (tv) {
    uint32_t active_tab = lv_tabview_get_tab_act(tv);
    for (int i = 0; i < 5; i++) {
      if (bottom_nav_btns[i]) {
        if (i == active_tab) {
          lv_obj_set_style_border_color(bottom_nav_btns[i], color, 0);
        }
      }
    }
  }

  /* Charts */
  if (chart1 && ser1)
    lv_chart_set_series_color(chart1, ser1, color);
  if (chart2 && ser3)
    lv_chart_set_series_color(chart2, ser3, color);

  /* System Color manager container */
  if (color_changer_cont) {
    lv_obj_set_style_border_color(color_changer_cont, color, 0);
  }

  lv_obj_invalidate(lv_scr_act());
}

static void color_changer_event_cb(lv_event_t *e) {
  if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
    if (color_changer_swatches_cont && color_changer_title_cont) {
      bool is_hidden = lv_obj_has_flag(color_changer_swatches_cont, LV_OBJ_FLAG_HIDDEN);
      if (is_hidden) {
        /* Expand swatches, hide "System Color" text */
        lv_obj_clear_flag(color_changer_swatches_cont, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(color_changer_title_cont, LV_OBJ_FLAG_HIDDEN);
      } else {
        /* Collapse swatches, show "System Color" text */
        lv_obj_add_flag(color_changer_swatches_cont, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(color_changer_title_cont, LV_OBJ_FLAG_HIDDEN);
      }
    }
  }
}

static void color_swatch_event_cb(lv_event_t *e) {
  if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
    uint32_t *color_hex_ptr = (uint32_t *)lv_event_get_user_data(e);
    if (color_hex_ptr) {
      update_accent_color(lv_color_hex(*color_hex_ptr));
    }
    /* Collapse swatches and show "System Color" text after picking a color */
    if (color_changer_swatches_cont && color_changer_title_cont) {
      lv_obj_add_flag(color_changer_swatches_cont, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(color_changer_title_cont, LV_OBJ_FLAG_HIDDEN);
    }
  }
}

static void color_changer_create(lv_obj_t *parent) {
  /* Squarish container right under Display Settings title */
  lv_obj_t *cont = lv_obj_create(parent);
  color_changer_cont = cont;
  lv_obj_set_size(cont, lv_pct(100), LV_SIZE_CONTENT);
  lv_obj_set_style_pad_all(cont, 10, 0);
  lv_obj_set_style_bg_color(cont, lv_color_hex(0x161D2B), 0);
  lv_obj_set_style_border_color(cont, current_accent_color, 0);
  lv_obj_set_style_border_width(cont, 1, 0);
  lv_obj_set_style_radius(cont, 8, 0); /* Squarish outer border */
  lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(cont, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(cont, color_changer_event_cb, LV_EVENT_CLICKED, NULL);

  /* Title container (Collapsed view): Drop icon + "System Color" text */
  lv_obj_t *title_cont = lv_obj_create(cont);
  color_changer_title_cont = title_cont;
  lv_obj_remove_style_all(title_cont);
  lv_obj_set_size(title_cont, lv_pct(100), LV_SIZE_CONTENT);
  lv_obj_set_flex_flow(title_cont, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(title_cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_column(title_cont, 8, 0);
  lv_obj_clear_flag(title_cont, LV_OBJ_FLAG_CLICKABLE);

  /* Drop Icon */
  lv_obj_t *drop_icn = lv_label_create(title_cont);
  lv_label_set_text(drop_icn, LV_SYMBOL_TINT);
  lv_obj_add_style(drop_icn, &style_icon, 0);
  lv_obj_clear_flag(drop_icn, LV_OBJ_FLAG_CLICKABLE);

  /* "System Color" Label */
  lv_obj_t *title_lbl = lv_label_create(title_cont);
  lv_label_set_text(title_lbl, "System Color");
  lv_obj_add_style(title_lbl, &style_title, 0);
  lv_obj_clear_flag(title_lbl, LV_OBJ_FLAG_CLICKABLE);

  /* Color Swatches container (Expanded view): 7 color circles covering the title */
  lv_obj_t *swatches_cont = lv_obj_create(cont);
  color_changer_swatches_cont = swatches_cont;
  lv_obj_remove_style_all(swatches_cont);
  lv_obj_set_size(swatches_cont, lv_pct(100), LV_SIZE_CONTENT);
  lv_obj_set_flex_flow(swatches_cont, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(swatches_cont, LV_FLEX_ALIGN_SPACE_EVENLY,
                        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_add_flag(swatches_cont, LV_OBJ_FLAG_HIDDEN); /* Collapsed by default */
  lv_obj_clear_flag(swatches_cont, LV_OBJ_FLAG_CLICKABLE);

  for (uint32_t i = 0; i < 7; i++) {
    lv_obj_t *c = lv_btn_create(swatches_cont);
    lv_obj_set_style_bg_color(c, lv_color_hex(custom_colors[i]), 0);
    lv_obj_set_style_radius(c, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_size(c, 24, 24);
    lv_obj_add_event_cb(c, color_swatch_event_cb, LV_EVENT_CLICKED,
                        (void *)&custom_colors[i]);
    lv_obj_clear_flag(c, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
  }
}

/* ---- Permanent Bottom Navigation Bar (Rectangular / Square Style) ---- */

static void bottom_nav_btn_cb(lv_event_t *e) {
  uint32_t tab_idx = *(uint32_t *)lv_event_get_user_data(e);
  if (tv) {
    lv_tabview_set_act(tv, tab_idx, LV_ANIM_OFF);
  }
  for (int i = 0; i < 6; i++) {
    lv_obj_t *label = lv_obj_get_child(bottom_nav_btns[i], 0);
    if (i == tab_idx) {
      lv_obj_set_style_bg_color(bottom_nav_btns[i], lv_color_hex(0x161C28), 0);
      lv_obj_set_style_border_color(bottom_nav_btns[i], current_accent_color,
                                    0);
      lv_obj_set_style_border_side(bottom_nav_btns[i], LV_BORDER_SIDE_TOP, 0);
      lv_obj_set_style_border_width(bottom_nav_btns[i], 3, 0);
      if (label)
        lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), 0);
    } else {
      lv_obj_set_style_bg_color(bottom_nav_btns[i], lv_color_hex(0x090D14), 0);
      lv_obj_set_style_border_color(bottom_nav_btns[i], lv_color_hex(0x1F2836),
                                    0);
      lv_obj_set_style_border_side(bottom_nav_btns[i], LV_BORDER_SIDE_RIGHT, 0);
      lv_obj_set_style_border_width(bottom_nav_btns[i], 1, 0);
      if (label)
        lv_obj_set_style_text_color(label, lv_color_hex(0x566D85), 0);
    }
  }
}

static void bottom_nav_create(void) {
  static const char *tab_names[] = {"Profile", "Analytics", "Summary", "Settings",
                                    "Network", "SD Card"};
  static uint32_t tab_indices[] = {0, 1, 2, 3, 4, 5};

  lv_obj_t *bottom_nav = lv_obj_create(lv_scr_act());
  lv_obj_set_size(bottom_nav, LV_PCT(100), 31);
  lv_obj_align(bottom_nav, LV_ALIGN_BOTTOM_MID, 0, 0);
  lv_obj_set_style_pad_all(bottom_nav, 0, 0);
  lv_obj_set_style_pad_gap(bottom_nav, 0, 0);
  lv_obj_set_style_bg_color(bottom_nav, lv_color_hex(0x090D14), 0);
  lv_obj_set_style_bg_opa(bottom_nav, LV_OPA_COVER, 0);
  lv_obj_set_style_border_color(bottom_nav, lv_color_hex(0x1F2836), 0);
  lv_obj_set_style_border_side(bottom_nav, LV_BORDER_SIDE_TOP, 0);
  lv_obj_set_style_border_width(bottom_nav, 1, 0);
  lv_obj_clear_flag(bottom_nav, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_flex_flow(bottom_nav, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(bottom_nav, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);

  for (int i = 0; i < 6; i++) {
    lv_obj_t *btn = lv_btn_create(bottom_nav);
    bottom_nav_btns[i] = btn;
    lv_obj_set_height(btn, LV_PCT(100));
    lv_obj_set_flex_grow(btn, 1);
    lv_obj_set_style_radius(btn, 0, 0); /* Flat IDE tab style */
    lv_obj_set_style_pad_ver(btn, 4, 0);
    lv_obj_set_style_pad_hor(btn, 2, 0);
    lv_obj_set_style_shadow_width(btn, 0, 0);

    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, tab_names[i]);
    lv_label_set_long_mode(label, LV_LABEL_LONG_CLIP);
    lv_obj_center(label);

    if (i == 0) {
      lv_obj_set_style_bg_color(btn, lv_color_hex(0x161C28), 0);
      lv_obj_set_style_border_color(btn, lv_color_hex(0x38BDF8), 0);
      lv_obj_set_style_border_side(btn, LV_BORDER_SIDE_TOP, 0);
      lv_obj_set_style_border_width(btn, 3, 0);
      lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), 0);
    } else {
      lv_obj_set_style_bg_color(btn, lv_color_hex(0x090D14), 0);
      lv_obj_set_style_border_color(btn, lv_color_hex(0x1F2836), 0);
      lv_obj_set_style_border_side(btn, LV_BORDER_SIDE_RIGHT, 0);
      lv_obj_set_style_border_width(btn, 1, 0);
      lv_obj_set_style_text_color(label, lv_color_hex(0x566D85), 0);
    }

    lv_obj_add_event_cb(btn, bottom_nav_btn_cb, LV_EVENT_CLICKED,
                        &tab_indices[i]);
    lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
  }
}

static lv_obj_t *create_meter_box(lv_obj_t *parent, const char *title,
                                  const char *text1, const char *text2,
                                  const char *text3) {
  lv_obj_t *cont = lv_obj_create(parent);
  lv_obj_set_height(cont, LV_SIZE_CONTENT);
  lv_obj_set_flex_grow(cont, 1);

  lv_obj_t *title_label = lv_label_create(cont);
  lv_label_set_text(title_label, title);
  lv_obj_add_style(title_label, &style_title, 0);

  lv_obj_t *meter = lv_meter_create(cont);
  lv_obj_remove_style(meter, NULL, LV_PART_MAIN);
  lv_obj_remove_style(meter, NULL, LV_PART_INDICATOR);
  lv_obj_set_width(meter, LV_PCT(100));

  lv_obj_t *bullet1 = lv_obj_create(cont);
  lv_obj_set_size(bullet1, 13, 13);
  lv_obj_remove_style(bullet1, NULL, LV_PART_SCROLLBAR);
  lv_obj_add_style(bullet1, &style_bullet, 0);
  lv_obj_set_style_bg_color(bullet1, lv_palette_main(LV_PALETTE_RED), 0);
  lv_obj_t *label1 = lv_label_create(cont);
  lv_label_set_text(label1, text1);

  lv_obj_t *bullet2 = lv_obj_create(cont);
  lv_obj_set_size(bullet2, 13, 13);
  lv_obj_remove_style(bullet2, NULL, LV_PART_SCROLLBAR);
  lv_obj_add_style(bullet2, &style_bullet, 0);
  lv_obj_set_style_bg_color(bullet2, lv_palette_main(LV_PALETTE_BLUE), 0);
  lv_obj_t *label2 = lv_label_create(cont);
  lv_label_set_text(label2, text2);

  lv_obj_t *bullet3 = lv_obj_create(cont);
  lv_obj_set_size(bullet3, 13, 13);
  lv_obj_remove_style(bullet3, NULL, LV_PART_SCROLLBAR);
  lv_obj_add_style(bullet3, &style_bullet, 0);
  lv_obj_set_style_bg_color(bullet3, lv_palette_main(LV_PALETTE_GREEN), 0);
  lv_obj_t *label3 = lv_label_create(cont);
  lv_label_set_text(label3, text3);

  if (disp_size == DISP_MEDIUM) {
    static lv_coord_t grid_col_dsc[] = {LV_GRID_CONTENT, LV_GRID_FR(1),
                                        LV_GRID_CONTENT, LV_GRID_FR(8),
                                        LV_GRID_TEMPLATE_LAST};
    static lv_coord_t grid_row_dsc[] = {
        LV_GRID_CONTENT, LV_GRID_FR(1), LV_GRID_CONTENT,      LV_GRID_CONTENT,
        LV_GRID_CONTENT, LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};

    lv_obj_set_grid_dsc_array(cont, grid_col_dsc, grid_row_dsc);
    lv_obj_set_grid_cell(title_label, LV_GRID_ALIGN_START, 0, 4,
                         LV_GRID_ALIGN_START, 0, 1);
    lv_obj_set_grid_cell(meter, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_START,
                         1, 3);
    lv_obj_set_grid_cell(bullet1, LV_GRID_ALIGN_START, 2, 1,
                         LV_GRID_ALIGN_CENTER, 2, 1);
    lv_obj_set_grid_cell(bullet2, LV_GRID_ALIGN_START, 2, 1,
                         LV_GRID_ALIGN_CENTER, 3, 1);
    lv_obj_set_grid_cell(bullet3, LV_GRID_ALIGN_START, 2, 1,
                         LV_GRID_ALIGN_CENTER, 4, 1);
    lv_obj_set_grid_cell(label1, LV_GRID_ALIGN_STRETCH, 3, 1,
                         LV_GRID_ALIGN_CENTER, 2, 1);
    lv_obj_set_grid_cell(label2, LV_GRID_ALIGN_STRETCH, 3, 1,
                         LV_GRID_ALIGN_CENTER, 3, 1);
    lv_obj_set_grid_cell(label3, LV_GRID_ALIGN_STRETCH, 3, 1,
                         LV_GRID_ALIGN_CENTER, 4, 1);
  } else {
    static lv_coord_t grid_col_dsc[] = {LV_GRID_CONTENT, LV_GRID_FR(1),
                                        LV_GRID_TEMPLATE_LAST};
    static lv_coord_t grid_row_dsc[] = {LV_GRID_CONTENT, LV_GRID_CONTENT,
                                        LV_GRID_CONTENT, LV_GRID_CONTENT,
                                        LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST};
    lv_obj_set_grid_dsc_array(cont, grid_col_dsc, grid_row_dsc);
    lv_obj_set_grid_cell(title_label, LV_GRID_ALIGN_START, 0, 2,
                         LV_GRID_ALIGN_START, 0, 1);
    lv_obj_set_grid_cell(meter, LV_GRID_ALIGN_START, 0, 2, LV_GRID_ALIGN_START,
                         1, 1);
    lv_obj_set_grid_cell(bullet1, LV_GRID_ALIGN_START, 0, 1,
                         LV_GRID_ALIGN_START, 2, 1);
    lv_obj_set_grid_cell(bullet2, LV_GRID_ALIGN_START, 0, 1,
                         LV_GRID_ALIGN_START, 3, 1);
    lv_obj_set_grid_cell(bullet3, LV_GRID_ALIGN_START, 0, 1,
                         LV_GRID_ALIGN_START, 4, 1);
    lv_obj_set_grid_cell(label1, LV_GRID_ALIGN_STRETCH, 1, 1,
                         LV_GRID_ALIGN_START, 2, 1);
    lv_obj_set_grid_cell(label2, LV_GRID_ALIGN_STRETCH, 1, 1,
                         LV_GRID_ALIGN_START, 3, 1);
    lv_obj_set_grid_cell(label3, LV_GRID_ALIGN_STRETCH, 1, 1,
                         LV_GRID_ALIGN_START, 4, 1);
  }

  return meter;
}

static lv_obj_t *create_shop_item(lv_obj_t *parent, const void *img_src,
                                  const char *name, const char *category,
                                  const char *price) {
  static lv_coord_t grid_col_dsc[] = {LV_GRID_CONTENT, 5, LV_GRID_FR(1),
                                      LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
  static lv_coord_t grid_row_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1),
                                      LV_GRID_TEMPLATE_LAST};

  lv_obj_t *cont = lv_obj_create(parent);
  lv_obj_remove_style_all(cont);
  lv_obj_set_size(cont, LV_PCT(100), LV_SIZE_CONTENT);
  lv_obj_set_grid_dsc_array(cont, grid_col_dsc, grid_row_dsc);

  lv_obj_t *img = lv_img_create(cont);
  lv_img_set_src(img, img_src);
  lv_obj_set_grid_cell(img, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_START, 0,
                       2);

  lv_obj_t *label;
  label = lv_label_create(cont);
  lv_label_set_text(label, name);
  lv_obj_set_grid_cell(label, LV_GRID_ALIGN_START, 2, 1, LV_GRID_ALIGN_END, 0,
                       1);

  label = lv_label_create(cont);
  lv_label_set_text(label, category);
  lv_obj_add_style(label, &style_text_muted, 0);
  lv_obj_set_grid_cell(label, LV_GRID_ALIGN_START, 2, 1, LV_GRID_ALIGN_START, 1,
                       1);

  label = lv_label_create(cont);
  lv_label_set_text(label, price);
  lv_obj_set_grid_cell(label, LV_GRID_ALIGN_END, 3, 1, LV_GRID_ALIGN_END, 0, 1);

  return cont;
}

static void ta_event_cb(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t *ta = lv_event_get_target(e);
  lv_obj_t *kb = lv_event_get_user_data(e);
  if (code == LV_EVENT_FOCUSED) {
    if (lv_indev_get_type(lv_indev_get_act()) != LV_INDEV_TYPE_KEYPAD) {
      lv_keyboard_set_textarea(kb, ta);
      lv_obj_set_style_max_height(kb, LV_HOR_RES * 2 / 3, 0);
      lv_obj_update_layout(tv); /*Be sure the sizes are recalculated*/
      lv_obj_set_height(tv, LV_VER_RES - lv_obj_get_height(kb));
      lv_obj_clear_flag(kb, LV_OBJ_FLAG_HIDDEN);
      lv_obj_scroll_to_view_recursive(ta, LV_ANIM_OFF);
    }
  } else if (code == LV_EVENT_DEFOCUSED) {
    lv_keyboard_set_textarea(kb, NULL);
    lv_obj_set_height(tv, LV_VER_RES);
    lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
    lv_indev_reset(NULL, ta);

  } else if (code == LV_EVENT_READY || code == LV_EVENT_CANCEL) {
    lv_obj_set_height(tv, LV_VER_RES);
    lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_state(ta, LV_STATE_FOCUSED);
    lv_indev_reset(
        NULL,
        ta); /*To forget the last clicked object to make it focusable again*/
  }
}

static void birthday_event_cb(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t *ta = lv_event_get_target(e);

  if (code == LV_EVENT_FOCUSED) {
    if (lv_indev_get_type(lv_indev_get_act()) == LV_INDEV_TYPE_POINTER) {
      if (calendar == NULL) {
        lv_obj_add_flag(lv_layer_top(), LV_OBJ_FLAG_CLICKABLE);
        calendar = lv_calendar_create(lv_layer_top());
        lv_obj_set_style_bg_opa(lv_layer_top(), LV_OPA_50, 0);
        lv_obj_set_style_bg_color(lv_layer_top(),
                                  lv_palette_main(LV_PALETTE_GREY), 0);
        if (disp_size == DISP_SMALL)
          lv_obj_set_size(calendar, 180, 200);
        else if (disp_size == DISP_MEDIUM)
          lv_obj_set_size(calendar, 200, 220);
        else
          lv_obj_set_size(calendar, 300, 330);
        lv_calendar_set_showed_date(calendar, 1990, 01);
        lv_obj_align(calendar, LV_ALIGN_CENTER, 0, 30);
        lv_obj_add_event_cb(calendar, calendar_event_cb, LV_EVENT_ALL, ta);

        lv_calendar_header_dropdown_create(calendar);
      }
    }
  }
}

static void calendar_event_cb(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t *ta = lv_event_get_user_data(e);
  lv_obj_t *obj = lv_event_get_current_target(e);
  if (code == LV_EVENT_VALUE_CHANGED) {
    lv_calendar_date_t d;
    lv_calendar_get_pressed_date(obj, &d);
    char buf[32];
    lv_snprintf(buf, sizeof(buf), "%02d.%02d.%d", d.day, d.month, d.year);
    lv_textarea_set_text(ta, buf);

    lv_obj_del(calendar);
    calendar = NULL;
    lv_obj_clear_flag(lv_layer_top(), LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_bg_opa(lv_layer_top(), LV_OPA_TRANSP, 0);
  }
}

static void slider_event_cb(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t *obj = lv_event_get_target(e);

  if (code == LV_EVENT_REFR_EXT_DRAW_SIZE) {
    lv_coord_t *s = lv_event_get_param(e);
    *s = LV_MAX(*s, 60);
  } else if (code == LV_EVENT_DRAW_PART_END) {
    lv_obj_draw_part_dsc_t *dsc = lv_event_get_param(e);
    if (dsc->part == LV_PART_KNOB && lv_obj_has_state(obj, LV_STATE_PRESSED)) {
      char buf[8];
      lv_snprintf(buf, sizeof(buf), "%" LV_PRId32, lv_slider_get_value(obj));

      lv_point_t text_size;
      lv_txt_get_size(&text_size, buf, font_normal, 0, 0, LV_COORD_MAX,
                      LV_TEXT_FLAG_NONE);

      lv_area_t txt_area;
      txt_area.x1 = dsc->draw_area->x1 + lv_area_get_width(dsc->draw_area) / 2 -
                    text_size.x / 2;
      txt_area.x2 = txt_area.x1 + text_size.x;
      txt_area.y2 = dsc->draw_area->y1 - 10;
      txt_area.y1 = txt_area.y2 - text_size.y;

      lv_area_t bg_area;
      bg_area.x1 = txt_area.x1 - LV_DPX(8);
      bg_area.x2 = txt_area.x2 + LV_DPX(8);
      bg_area.y1 = txt_area.y1 - LV_DPX(8);
      bg_area.y2 = txt_area.y2 + LV_DPX(8);

      lv_draw_rect_dsc_t rect_dsc;
      lv_draw_rect_dsc_init(&rect_dsc);
      rect_dsc.bg_color = lv_palette_darken(LV_PALETTE_GREY, 3);
      rect_dsc.radius = LV_DPX(5);
      lv_draw_rect(dsc->draw_ctx, &rect_dsc, &bg_area);

      lv_draw_label_dsc_t label_dsc;
      lv_draw_label_dsc_init(&label_dsc);
      label_dsc.color = lv_color_white();
      label_dsc.font = font_normal;
      lv_draw_label(dsc->draw_ctx, &label_dsc, &txt_area, buf, NULL);
    }
  }
}

static void chart_event_cb(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t *obj = lv_event_get_target(e);

  if (code == LV_EVENT_PRESSED || code == LV_EVENT_RELEASED) {
    lv_obj_invalidate(obj); /*To make the value boxes visible*/
  } else if (code == LV_EVENT_DRAW_PART_BEGIN) {
    lv_obj_draw_part_dsc_t *dsc = lv_event_get_param(e);
    /*Set the markers' text*/
    if (dsc->part == LV_PART_TICKS && dsc->id == LV_CHART_AXIS_PRIMARY_X) {
      if (lv_chart_get_type(obj) == LV_CHART_TYPE_BAR) {
        const char *month[] = {"I",   "II",   "III", "IV", "V",  "VI",
                               "VII", "VIII", "IX",  "X",  "XI", "XII"};
        lv_snprintf(dsc->text, dsc->text_length, "%s", month[dsc->value]);
      } else {
        const char *month[] = {"Jan",  "Febr", "March", "Apr", "May", "Jun",
                               "July", "Aug",  "Sept",  "Oct", "Nov", "Dec"};
        lv_snprintf(dsc->text, dsc->text_length, "%s", month[dsc->value]);
      }
    }

    /*Add the faded area before the lines are drawn */
    else if (dsc->part == LV_PART_ITEMS) {
#if LV_DRAW_COMPLEX
      /*Add  a line mask that keeps the area below the line*/
      if (dsc->p1 && dsc->p2) {
        lv_draw_mask_line_param_t line_mask_param;
        lv_draw_mask_line_points_init(&line_mask_param, dsc->p1->x, dsc->p1->y,
                                      dsc->p2->x, dsc->p2->y,
                                      LV_DRAW_MASK_LINE_SIDE_BOTTOM);
        int16_t line_mask_id = lv_draw_mask_add(&line_mask_param, NULL);

        /*Add a fade effect: transparent bottom covering top*/
        lv_coord_t h = lv_obj_get_height(obj);
        lv_draw_mask_fade_param_t fade_mask_param;
        lv_draw_mask_fade_init(&fade_mask_param, &obj->coords, LV_OPA_COVER,
                               obj->coords.y1 + h / 8, LV_OPA_TRANSP,
                               obj->coords.y2);
        int16_t fade_mask_id = lv_draw_mask_add(&fade_mask_param, NULL);

        /*Draw a rectangle that will be affected by the mask*/
        lv_draw_rect_dsc_t draw_rect_dsc;
        lv_draw_rect_dsc_init(&draw_rect_dsc);
        draw_rect_dsc.bg_opa = LV_OPA_50;
        draw_rect_dsc.bg_color = dsc->line_dsc->color;

        lv_area_t obj_clip_area;
        _lv_area_intersect(&obj_clip_area, dsc->draw_ctx->clip_area,
                           &obj->coords);
        const lv_area_t *clip_area_ori = dsc->draw_ctx->clip_area;
        dsc->draw_ctx->clip_area = &obj_clip_area;
        lv_area_t a;
        a.x1 = dsc->p1->x;
        a.x2 = dsc->p2->x - 1;
        a.y1 = LV_MIN(dsc->p1->y, dsc->p2->y);
        a.y2 = obj->coords.y2;
        lv_draw_rect(dsc->draw_ctx, &draw_rect_dsc, &a);
        dsc->draw_ctx->clip_area = clip_area_ori;
        /*Remove the masks*/
        lv_draw_mask_remove_id(line_mask_id);
        lv_draw_mask_remove_id(fade_mask_id);
      }
#endif

      const lv_chart_series_t *ser = dsc->sub_part_ptr;

      if (lv_chart_get_pressed_point(obj) == dsc->id) {
        if (lv_chart_get_type(obj) == LV_CHART_TYPE_LINE) {
          dsc->rect_dsc->outline_color = lv_color_white();
          dsc->rect_dsc->outline_width = 2;
        } else {
          dsc->rect_dsc->shadow_color = ser->color;
          dsc->rect_dsc->shadow_width = 15;
          dsc->rect_dsc->shadow_spread = 0;
        }

        char buf[8];
        lv_snprintf(buf, sizeof(buf), "%" LV_PRIu32, dsc->value);

        lv_point_t text_size;
        lv_txt_get_size(&text_size, buf, font_normal, 0, 0, LV_COORD_MAX,
                        LV_TEXT_FLAG_NONE);

        lv_area_t txt_area;
        if (lv_chart_get_type(obj) == LV_CHART_TYPE_BAR) {
          txt_area.y2 = dsc->draw_area->y1 - LV_DPX(15);
          txt_area.y1 = txt_area.y2 - text_size.y;
          if (ser == lv_chart_get_series_next(obj, NULL)) {
            txt_area.x1 =
                dsc->draw_area->x1 + lv_area_get_width(dsc->draw_area) / 2;
            txt_area.x2 = txt_area.x1 + text_size.x;
          } else {
            txt_area.x2 =
                dsc->draw_area->x1 + lv_area_get_width(dsc->draw_area) / 2;
            txt_area.x1 = txt_area.x2 - text_size.x;
          }
        } else {
          txt_area.x1 = dsc->draw_area->x1 +
                        lv_area_get_width(dsc->draw_area) / 2 - text_size.x / 2;
          txt_area.x2 = txt_area.x1 + text_size.x;
          txt_area.y2 = dsc->draw_area->y1 - LV_DPX(15);
          txt_area.y1 = txt_area.y2 - text_size.y;
        }

        lv_area_t bg_area;
        bg_area.x1 = txt_area.x1 - LV_DPX(8);
        bg_area.x2 = txt_area.x2 + LV_DPX(8);
        bg_area.y1 = txt_area.y1 - LV_DPX(8);
        bg_area.y2 = txt_area.y2 + LV_DPX(8);

        lv_draw_rect_dsc_t rect_dsc;
        lv_draw_rect_dsc_init(&rect_dsc);
        rect_dsc.bg_color = ser->color;
        rect_dsc.radius = LV_DPX(5);
        lv_draw_rect(dsc->draw_ctx, &rect_dsc, &bg_area);

        lv_draw_label_dsc_t label_dsc;
        lv_draw_label_dsc_init(&label_dsc);
        label_dsc.color = lv_color_white();
        label_dsc.font = font_normal;
        lv_draw_label(dsc->draw_ctx, &label_dsc, &txt_area, buf, NULL);
      } else {
        dsc->rect_dsc->outline_width = 0;
        dsc->rect_dsc->shadow_width = 0;
      }
    }
  }
}

static void shop_chart_event_cb(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_DRAW_PART_BEGIN) {
    lv_obj_draw_part_dsc_t *dsc = lv_event_get_param(e);
    /*Set the markers' text*/
    if (dsc->part == LV_PART_TICKS && dsc->id == LV_CHART_AXIS_PRIMARY_X) {
      const char *month[] = {"Jan",  "Febr", "March", "Apr", "May", "Jun",
                             "July", "Aug",  "Sept",  "Oct", "Nov", "Dec"};
      lv_snprintf(dsc->text, dsc->text_length, "%s", month[dsc->value]);
    }
    if (dsc->part == LV_PART_ITEMS) {
      dsc->rect_dsc->bg_opa = LV_OPA_TRANSP; /*We will draw it later*/
    }
  }
  if (code == LV_EVENT_DRAW_PART_END) {
    lv_obj_draw_part_dsc_t *dsc = lv_event_get_param(e);
    /*Add the faded area before the lines are drawn */
    if (dsc->part == LV_PART_ITEMS) {
      static const uint32_t devices[10] = {32, 43, 21, 56, 29,
                                           36, 19, 25, 62, 35};
      static const uint32_t clothes[10] = {12, 19, 23, 31, 27,
                                           32, 32, 11, 21, 32};
      static const uint32_t services[10] = {56, 38, 56, 13, 44,
                                            32, 49, 64, 17, 33};

      lv_draw_rect_dsc_t draw_rect_dsc;
      lv_draw_rect_dsc_init(&draw_rect_dsc);

      lv_coord_t h = lv_area_get_height(dsc->draw_area);

      lv_area_t a;
      a.x1 = dsc->draw_area->x1;
      a.x2 = dsc->draw_area->x2;

      a.y1 = dsc->draw_area->y1;
      a.y2 =
          a.y1 + 4 + (devices[dsc->id] * h) / 100; /*+4 to overlap the radius*/
      draw_rect_dsc.bg_color = lv_palette_main(LV_PALETTE_RED);
      draw_rect_dsc.radius = 4;
      lv_draw_rect(dsc->draw_ctx, &draw_rect_dsc, &a);

      a.y1 = a.y2 - 4; /*-4 to overlap the radius*/
      a.y2 = a.y1 + (clothes[dsc->id] * h) / 100;
      draw_rect_dsc.bg_color = lv_palette_main(LV_PALETTE_BLUE);
      draw_rect_dsc.radius = 0;
      lv_draw_rect(dsc->draw_ctx, &draw_rect_dsc, &a);

      a.y1 = a.y2;
      a.y2 = a.y1 + (services[dsc->id] * h) / 100;
      draw_rect_dsc.bg_color = lv_palette_main(LV_PALETTE_GREEN);
      lv_draw_rect(dsc->draw_ctx, &draw_rect_dsc, &a);
    }
  }
}

static void meter1_indic1_anim_cb(void *var, int32_t v) {
  lv_meter_set_indicator_end_value(meter1, var, v);

  lv_obj_t *card = lv_obj_get_parent(meter1);
  lv_obj_t *label = lv_obj_get_child(card, -5);
  lv_label_set_text_fmt(label, "Revenue: %" LV_PRId32 " %%", v);
}

static void meter1_indic2_anim_cb(void *var, int32_t v) {
  lv_meter_set_indicator_end_value(meter1, var, v);

  lv_obj_t *card = lv_obj_get_parent(meter1);
  lv_obj_t *label = lv_obj_get_child(card, -3);
  lv_label_set_text_fmt(label, "Sales: %" LV_PRId32 " %%", v);
}

static void meter1_indic3_anim_cb(void *var, int32_t v) {
  lv_meter_set_indicator_end_value(meter1, var, v);

  lv_obj_t *card = lv_obj_get_parent(meter1);
  lv_obj_t *label = lv_obj_get_child(card, -1);
  lv_label_set_text_fmt(label, "Costs: %" LV_PRId32 " %%", v);
}

static void profile_soc_timer_cb(lv_timer_t *timer) {
  LV_UNUSED(timer);

  if (profile_soc_down) {
    profile_soc_val--;
    if (profile_soc_val <= 5) {
      profile_soc_val = 5;
      profile_soc_down = false;
    }
  } else {
    profile_soc_val++;
    if (profile_soc_val >= 100) {
      profile_soc_val = 100;
      profile_soc_down = true;
    }
  }

  /* Voltage range: Min 260V (0%), Max 348V (100%) */
  float voltage = 260.0f + ((float)profile_soc_val / 100.0f) * (348.0f - 260.0f);

  /* Current logic: +1.8A when charging (increasing), -1.8A when discharging (decreasing) */
  float current_amps = profile_soc_down ? -1.8f : 1.8f;
  float power_watts = voltage * 1.8f;

  /* Warning red color (0xDC2626) when < 20%, theme color otherwise */
  lv_color_t active_color;
  if (profile_soc_val < 20) {
    active_color = lv_color_hex(0xDC2626);
  } else {
    active_color = current_accent_color;
  }

  if (profile_avatar) {
    lv_arc_set_value(profile_avatar, profile_soc_val);
    lv_obj_set_style_arc_color(profile_avatar, active_color, LV_PART_INDICATOR);
  }

  if (profile_pct_label) {
    lv_label_set_text_fmt(profile_pct_label, "%" LV_PRId32 "%%", profile_soc_val);
    lv_obj_set_style_text_color(profile_pct_label, active_color, 0);
  }

  /* Live updates for sub-boxes */
  if (profile_sub_box_val_labels[0]) {
    lv_label_set_text_fmt(profile_sub_box_val_labels[0], "%.1fV", (double)voltage);
  }
  if (profile_sub_box_val_labels[1]) {
    lv_label_set_text_fmt(profile_sub_box_val_labels[1], "%" LV_PRId32 "%%", profile_soc_val);
  }
  if (profile_sub_box_val_labels[2]) {
    if (current_amps > 0.0f) {
      lv_label_set_text_fmt(profile_sub_box_val_labels[2], "+%.1fA", (double)current_amps);
    } else {
      lv_label_set_text_fmt(profile_sub_box_val_labels[2], "%.1fA", (double)current_amps);
    }
  }
  if (profile_sub_box_val_labels[4]) {
    lv_label_set_text_fmt(profile_sub_box_val_labels[4], "%.1fW", (double)power_watts);
  }
}

static void meter2_timer_cb(lv_timer_t *timer) {
  lv_meter_indicator_t **indics = timer->user_data;

  static bool down1 = false;
  static bool down2 = false;
  static bool down3 = false;

  if (down1) {
    session_desktop -= 137;
    if (session_desktop < 1400)
      down1 = false;
  } else {
    session_desktop += 116;
    if (session_desktop > 4500)
      down1 = true;
  }

  if (down2) {
    session_tablet -= 3;
    if (session_tablet < 1400)
      down2 = false;
  } else {
    session_tablet += 9;
    if (session_tablet > 4500)
      down2 = true;
  }

  if (down3) {
    session_mobile -= 57;
    if (session_mobile < 1400)
      down3 = false;
  } else {
    session_mobile += 76;
    if (session_mobile > 4500)
      down3 = true;
  }

  uint32_t all = session_desktop + session_tablet + session_mobile;
  uint32_t pct1 = (session_desktop * 97) / all;
  uint32_t pct2 = (session_tablet * 97) / all;

  lv_meter_set_indicator_start_value(meter2, indics[0], 0);
  lv_meter_set_indicator_end_value(meter2, indics[0], pct1);

  lv_meter_set_indicator_start_value(meter2, indics[1], pct1 + 1);
  lv_meter_set_indicator_end_value(meter2, indics[1], pct1 + 1 + pct2);

  lv_meter_set_indicator_start_value(meter2, indics[2], pct1 + 1 + pct2 + 1);
  lv_meter_set_indicator_end_value(meter2, indics[2], 99);

  lv_obj_t *card = lv_obj_get_parent(meter2);
  lv_obj_t *label;

  label = lv_obj_get_child(card, -5);
  lv_label_set_text_fmt(label, "Desktop: %" LV_PRIu32, session_desktop);

  label = lv_obj_get_child(card, -3);
  lv_label_set_text_fmt(label, "Tablet: %" LV_PRIu32, session_tablet);

  label = lv_obj_get_child(card, -1);
  lv_label_set_text_fmt(label, "Mobile: %" LV_PRIu32, session_mobile);
}

static void meter3_anim_cb(void *var, int32_t v) {
  lv_meter_set_indicator_value(meter3, var, v);

  lv_obj_t *label = lv_obj_get_child(meter3, 0);
  lv_label_set_text_fmt(label, "%" LV_PRId32, v);
}

#endif
