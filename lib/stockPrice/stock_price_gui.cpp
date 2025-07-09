#include "stock_price_gui.h"



static char saved_ssid[64] = "default_ssid";
static char saved_pass[64] = "default_pass";

static char s_Symbols[4][5] = {"NVDA", "AAPL", "GOOG", "TSLA"};
static char s_Prices[4][10] = {"$0", "$0", "$0", "$0"};

// Widgets for AP settings
static lv_obj_t *ssid_ta;
static lv_obj_t *pass_ta;
static lv_obj_t *table;
static lv_obj_t *tabview;
static lv_obj_t *status_label;

// Loading screen widgets
static lv_obj_t *loading_layer = NULL;
static lv_obj_t *loading_label = NULL;
static lv_obj_t *loading_spinner = NULL;

// Forward declarations
static void save_btn_event_cb(lv_event_t *e);
static void ta_event_cb(lv_event_t *e);
static void reset_btn_event_cb(lv_event_t *e);

/***************************
 * ABSTRACT FUNCTIONS
 * These functions should be overridden in the apploication context
 ***************************/

wifi_ap_info_t load_ap_settings(void) __attribute__((weak));
void save_ap_settings(const char *ssid, const char *pass) __attribute__((weak));
void update_btn_event_cb(lv_event_t *e) __attribute__((weak));
void sw_reset(lv_event_t *e) __attribute__((weak));
/**
 * This function should load the saved SSID and password from persistent storage.
 * If it is the first time to laod, the ssid should be "testssid".
 */
wifi_ap_info_t load_ap_settings(void)
{

    wifi_ap_info_t data = {
        .ssid = saved_ssid,
        .pass = saved_pass};

    // LV_LOG_USER("Loaded SSID: %s, Password: %s", data.ssid, data.pass);

    return data;
}

// Helper to save settings (simulate persistent storage)
void save_ap_settings(const char *ssid, const char *pass)
{
}

void update_btn_event_cb(lv_event_t *e)
{
    LV_UNUSED(e);
}
/***************************
 * GUI FUNCTIONS
 ***************************/

// Home tab: Stock price table
static void create_home_tab(lv_obj_t *parent)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, "Stock Price Dashboard");
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 10);

    // Table for stock prices
    table = lv_table_create(parent);
    lv_obj_set_size(table, 220, 120);
    lv_obj_align(table, LV_ALIGN_TOP_MID, 0, 40);

    // Set table headers
    lv_table_set_cell_value(table, 0, 0, "Symbol");
    lv_table_set_cell_value(table, 0, 1, "Price");

    for (int i = 0; i < 4; ++i)
    {
        lv_table_set_cell_value(table, i + 1, 0, s_Symbols[i]);
        lv_table_set_cell_value(table, i + 1, 1, s_Prices[i]);
    }

    // Create a horizontal container for the two status labels
    lv_obj_t *status_row = lv_obj_create(parent);
    lv_obj_remove_style_all(status_row); // Remove default background and border
    lv_obj_set_size(status_row, 220, 30);
    lv_obj_align(status_row, LV_ALIGN_BOTTOM_MID, 0, -35);
    lv_obj_set_flex_flow(status_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_row(status_row, 0, 0);
    lv_obj_set_style_pad_column(status_row, 10, 0); // Space between labels

    // WiFi status label
    lv_obj_t *wifi_status_label = lv_label_create(status_row);
    lv_label_set_text(wifi_status_label, "WiFi status:");

    // Status label
    status_label = lv_label_create(status_row);
    lv_label_set_text(status_label, "Un-connected");
    lv_obj_set_style_text_color(status_label, lv_color_hex(0xFF0000), LV_PART_MAIN);

    lv_obj_t *update_btn = lv_button_create(parent);
    lv_obj_align(update_btn, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_t *btn_label = lv_label_create(update_btn);
    lv_label_set_text(btn_label, "Update");
    lv_obj_center(btn_label);
    lv_obj_add_event_cb(update_btn, update_btn_event_cb, LV_EVENT_CLICKED, NULL);
}

// AP Settings tab: Form for SSID, password, save button
static void create_ap_tab(lv_obj_t *parent)
{
    lv_obj_t *cont = lv_obj_create(parent);
    lv_obj_set_size(cont, lv_pct(100), lv_pct(100));
    lv_obj_center(cont);

    /*Create a keyboard*/
    lv_obj_t *kb = lv_keyboard_create(lv_screen_active());
    lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);

    lv_obj_t *ssid_label = lv_label_create(cont);
    lv_label_set_text(ssid_label, "SSID:");
    lv_obj_align(ssid_label, LV_ALIGN_TOP_LEFT, 10, 10);

    ssid_ta = lv_textarea_create(cont);
    lv_obj_set_width(ssid_ta, 100);
    lv_obj_set_height(ssid_ta, 32);
    lv_obj_align_to(ssid_ta, ssid_label, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
    lv_textarea_set_one_line(ssid_ta, true);
    lv_obj_add_event_cb(ssid_ta, ta_event_cb, LV_EVENT_ALL, kb);

    lv_obj_t *pass_label = lv_label_create(cont);
    lv_label_set_text(pass_label, "Password:");
    lv_obj_align(pass_label, LV_ALIGN_TOP_LEFT, 10, 50);

    pass_ta = lv_textarea_create(cont);
    lv_obj_set_width(pass_ta, 100);
    lv_textarea_set_password_mode(pass_ta, true);
    lv_obj_set_height(pass_ta, 32);
    lv_textarea_set_one_line(pass_ta, true);
    lv_obj_align_to(pass_ta, pass_label, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
    lv_obj_add_event_cb(pass_ta, ta_event_cb, LV_EVENT_ALL, kb);

    wifi_ap_info_t ap = load_ap_settings();
    // copy to local static variables
    memcpy(saved_ssid, ap.ssid, 64);
    memcpy(saved_pass, ap.pass, 64);

    lv_textarea_set_text(ssid_ta, saved_ssid);
    lv_textarea_set_text(pass_ta, saved_pass);

    // Create a horizontal container for Save and Reset buttons
    lv_obj_t *btn_row = lv_obj_create(cont);
    lv_obj_remove_style_all(btn_row);
    lv_obj_set_size(btn_row, 200, 40);
    lv_obj_align(btn_row, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_set_flex_flow(btn_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_row(btn_row, 0, 0);
    lv_obj_set_style_pad_column(btn_row, 10, 0);

    // Save button
    lv_obj_t *save_btn = lv_button_create(btn_row);
    lv_obj_set_size(save_btn, 80, 36);
    lv_obj_t *btn_label = lv_label_create(save_btn);
    lv_label_set_text(btn_label, "Save");
    lv_obj_center(btn_label);
    lv_obj_add_event_cb(save_btn, save_btn_event_cb, LV_EVENT_CLICKED, NULL);

    // Reset button
    lv_obj_t *reset_btn = lv_button_create(btn_row);
    lv_obj_set_size(reset_btn, 80, 36);
    lv_obj_t *reset_label = lv_label_create(reset_btn);
    lv_label_set_text(reset_label, "Reset");
    lv_obj_center(reset_label);
    lv_obj_add_event_cb(reset_btn, reset_btn_event_cb, LV_EVENT_CLICKED, NULL);
}

// Device Info tab
static void create_info_tab(lv_obj_t *parent)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text_fmt(label,
                          "Device Info\n"
                          "Firmware: v1.0.0\n"
                          "Framework: Arduino + LVGL 9.1\n"
                          "Board: ESP32-S3\n"
                          "Build Date: %s %s",
                          __DATE__, __TIME__);
    lv_obj_center(label);
}

// Save button event callback
static void save_btn_event_cb(lv_event_t *e)
{
    const char *ssid = lv_textarea_get_text(ssid_ta);
    const char *pass = lv_textarea_get_text(pass_ta);
    // Update the local static variables
    strcpy(saved_ssid, ssid);
    strcpy(saved_pass, pass);
    save_ap_settings(saved_ssid, saved_pass);

    // Show a message box (LVGL 9.1 API)
    // lv_obj_t *mbox = lv_msgbox_create(NULL);
    // lv_msgbox_add_text(mbox, "WiFi settings saved!");
    // lv_msgbox_add_close_button(mbox);
    // lv_obj_set_width(mbox, 180);
    // lv_obj_center(mbox);
}

// Add this callback function at file scope
static void reset_btn_event_cb(lv_event_t *e)
{

    // Call your storage reset and restart function
    sw_reset(e); // This should be defined in the application context
}

static void ta_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *ta = (lv_obj_t *)lv_event_get_target(e);
    lv_obj_t *kb = (lv_obj_t *)lv_event_get_user_data(e);
    if (code == LV_EVENT_FOCUSED)
    {
        if (lv_indev_get_type(lv_indev_active()) != LV_INDEV_TYPE_KEYPAD)
        {
            lv_keyboard_set_textarea(kb, ta);
            lv_obj_set_style_max_height(kb, LV_HOR_RES * 2 / 3, 0);
            lv_obj_update_layout(tabview); /*Be sure the sizes are recalculated*/
            // lv_obj_set_height(tabview, LV_VER_RES - lv_obj_get_height(kb));
            lv_obj_remove_flag(kb, LV_OBJ_FLAG_HIDDEN);
            lv_obj_scroll_to_view_recursive(ta, LV_ANIM_OFF);
            lv_indev_wait_release((lv_indev_t *)lv_event_get_param(e));
        }
    }
    else if (code == LV_EVENT_DEFOCUSED)
    {
        lv_keyboard_set_textarea(kb, NULL);
        lv_obj_set_height(tabview, LV_VER_RES);
        lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
        lv_indev_reset(NULL, ta);
    }
    else if (code == LV_EVENT_READY || code == LV_EVENT_CANCEL)
    {
        lv_obj_set_height(tabview, LV_VER_RES);
        lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
        lv_indev_reset(NULL, ta); /*To forget the last clicked object to make it focusable again*/
    }
}

/**
 *  CPP code
 */
void gui_code_init(void)
{
    // Create tabview
    tabview = lv_tabview_create(lv_screen_active());
    lv_obj_set_size(tabview, lv_pct(100), lv_pct(100));

    lv_obj_t *tab_home = lv_tabview_add_tab(tabview, "Home");
    lv_obj_t *tab_ap = lv_tabview_add_tab(tabview, "AP \nSettings");
    lv_obj_t *tab_info = lv_tabview_add_tab(tabview, "Device \nInfo");

    create_home_tab(tab_home);
    create_ap_tab(tab_ap);
    create_info_tab(tab_info);
}

char (*gui_get_symbol_list(void))[5]
{
    return s_Symbols;
}

char (*gui_get_price_list(void))[10]
{
    // Return a pointer to the static array of prices
    return s_Prices;
}

void gui_update_price(void)
{
    for (int i = 0; i < 4; ++i)
    {
        lv_table_set_cell_value(table, i + 1, 1, s_Prices[i]);
    }
}

void gui_update_wifi_status(const char *status)
{
   
    lv_label_set_text(status_label, status);
    if (strcmp(status, "Connected") == 0)
    {
        lv_obj_set_style_text_color(status_label, lv_color_hex(0x00FF00), LV_PART_MAIN);
    }
    else
    {
        lv_obj_set_style_text_color(status_label, lv_color_hex(0xFF0000), LV_PART_MAIN);
    }
}

void show_msg_box(const char *title, const char *msg)
{
    lv_obj_t *mbox = lv_msgbox_create(NULL);
    lv_msgbox_add_text(mbox, msg);
    lv_msgbox_add_close_button(mbox);
    lv_obj_set_width(mbox, lv_pct(90));
    lv_obj_center(mbox);
}

void show_btn_msg_box(const char *title, const char *msg, lv_event_cb_t btn_event_cb)
{
    lv_obj_t * mbox1 = lv_msgbox_create(NULL);

    lv_msgbox_add_title(mbox1, title);
    lv_obj_set_width(mbox1, lv_pct(90));

    lv_msgbox_add_text(mbox1, msg);
    lv_msgbox_add_close_button(mbox1);

    lv_obj_t * btn;
    btn = lv_msgbox_add_footer_button(mbox1, "Ok");
    lv_obj_add_event_cb(btn, btn_event_cb, LV_EVENT_CLICKED, NULL);

  
}

// Show loading overlay on top of current screen
void gui_show_loading_screen(const char *msg)
{
    if (loading_layer) return; // Already shown

    lv_obj_t *act_scr = lv_screen_active();
    loading_layer = lv_obj_create(act_scr);
    lv_obj_remove_style_all(loading_layer);
    lv_obj_set_size(loading_layer, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_color(loading_layer, lv_color_hex(0x222244), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(loading_layer, LV_OPA_80, LV_PART_MAIN); // semi-transparent

    // Create spinner
    loading_spinner = lv_spinner_create(loading_layer);
    lv_obj_set_size(loading_spinner, 40, 40);
    lv_obj_align(loading_spinner, LV_ALIGN_CENTER, 0, -30);
    // lv_obj_set_hidden(loading_spinner, false);

    // Create label
    loading_label = lv_label_create(loading_layer);
    lv_label_set_text(loading_label, msg);
    lv_obj_set_style_text_color(loading_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_align(loading_label, LV_ALIGN_CENTER, 0, 30);

    // Make sure the overlay is on top
    lv_obj_move_foreground(loading_layer);
}

// Update loading status text
void gui_update_loading_status(const char *msg)
{
    if (loading_label) {
        lv_label_set_text(loading_label, msg);
    }
}

// Hide loading overlay
void gui_hide_loading_screen(void)
{
    // if (loading_spinner) {
    //     lv_obj_set_hidden(loading_spinner, true);
    // }
    if (loading_layer) {
        lv_obj_del(loading_layer);
        loading_layer = NULL;
        loading_label = NULL;
        loading_spinner = NULL;
    }
}
