#ifndef GUI_H
#define GUI_H
#include "lvgl.h"
#ifdef __cplusplus
extern "C"
{
#endif
// Add your includes here
#include <stdint.h>

    // Define any constants, enums, or macros here

    // Declare your structs and typedefs here
    typedef struct
    {
        char *ssid;
        char *pass;
    } wifi_ap_info_t;

#ifdef __cplusplus
}
#endif

/**
 * CPP code
 */
void show_msg_box(const char *title, const char *msg);
void show_btn_msg_box(const char *title, const char *msg, lv_event_cb_t btn_event_cb);

void gui_code_init(void);
void gui_update_price(void);
void gui_update_wifi_status(const char *status);
char (*gui_get_symbol_list(void))[5];
char (*gui_get_price_list(void))[10];

//Loading api
void gui_show_loading_screen(const char *msg);
void gui_update_loading_status(const char *msg);
void gui_hide_loading_screen(void);

#endif
