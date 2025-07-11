
#include "app_hal.h"
#include "lvgl.h"

#include <Arduino.h>

#include "custom_disp.hpp"

static const uint32_t screenWidth = WIDTH;
static const uint32_t screenHeight = HEIGHT;

const unsigned int lvBufferSize = screenWidth * 30;
uint8_t lvBuffer[2][lvBufferSize];

static lv_display_t *lvDisplay;
static lv_indev_t *lvInput;

#if LV_USE_LOG != 0
static void lv_log_print_g_cb(lv_log_level_t level, const char *buf)
{
    LV_UNUSED(level);
    Serial.printf(buf, strlen(buf));
}
#endif

/* Display flushing */
void my_disp_flush(lv_display_t *display, const lv_area_t *area, unsigned char *data)
{

    uint32_t w = lv_area_get_width(area);
    uint32_t h = lv_area_get_height(area);
    lv_draw_sw_rgb565_swap(data, w * h);

    if (tft.getStartCount() == 0)
    {
        tft.endWrite();
    }
    tft.pushImageDMA(area->x1, area->y1, area->x2 - area->x1 + 1, area->y2 - area->y1 + 1, (uint16_t *)data);
    lv_display_flush_ready(display); /* tell lvgl that flushing is done */
}

/*Read the touchpad*/
void my_touchpad_read(lv_indev_t *indev_driver, lv_indev_data_t *data)
{
    uint16_t touchX, touchY;
    bool touched = tft.getTouch(&touchX, &touchY);
    if (!touched)
    {
        data->state = LV_INDEV_STATE_REL;
    }
    else
    {
        data->state = LV_INDEV_STATE_PR;
        /*Set the coordinates*/
        data->point.x = touchX;
        data->point.y = touchY;
        // Serial.print("Data x ");
        // Serial.print(touchX);

        // Serial.print(",Data y ");
        // Serial.println(touchY);
    }
}

/* Tick source, tell LVGL how much time (milliseconds) has passed */
static uint32_t my_tick(void)
{
    return millis();
}

static uint8_t *ext_color_buf;
static uint8_t *ext_color_buf2;

void hal_setup(void)
{
    Serial.println("Starting LVGL setup...");
    Serial.printf("LV definition: LV LOG level=%d, LV_USE_LOG=%d\n", LV_LOG_LEVEL, LV_USE_LOG);
    lv_init();
    /* Initialize the display drivers */
    tft.init();
    tft.setBrightness(0);
    tft.initDMA();
    // tft.startWrite();

    // tft.fillScreen(TFT_BLACK);

    /* Set display rotation to landscape */
    // tft.setRotation(1);

    /* Set the tick callback */
    lv_tick_set_cb(my_tick);

    /* Create LVGL display and set the flush function */
    lvDisplay = lv_display_create(screenWidth, screenHeight);
    lv_display_set_color_format(lvDisplay, LV_COLOR_FORMAT_RGB565);
    lv_display_set_flush_cb(lvDisplay, my_disp_flush);
    lv_display_set_buffers(lvDisplay, lvBuffer[0], lvBuffer[1], lvBufferSize, LV_DISPLAY_RENDER_MODE_PARTIAL);

#if LV_USE_LOG != 0
    lv_log_register_print_cb((lv_log_print_g_cb_t)lv_log_print_g_cb); /* register print function for debugging */
#endif

    /* Set the touch input function */
    lvInput = lv_indev_create();
    lv_indev_set_type(lvInput, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(lvInput, my_touchpad_read);

// Test PSRAM
#if 0
    LV_LOG_USER("befor:ESP.getFreeHeap():%d,PSRAM=%d,Free_PSRAM=%d", ESP.getFreeHeap(), ESP.getPsramSize(), ESP.getFreePsram());

    ext_color_buf = (uint8_t *)heap_caps_malloc(1024, MALLOC_CAP_SPIRAM);
    ext_color_buf2 = (uint8_t *)heap_caps_malloc(1024, MALLOC_CAP_SPIRAM);

    LV_LOG_USER("ext_color_buf:%d,ext_color_buf2=%d", ext_color_buf, ext_color_buf2);
    LV_LOG_USER("after malloc:ESP.getFreeHeap():%d,PSRAM=%d,Free_PSRAM=%d", ESP.getFreeHeap(), ESP.getPsramSize(), ESP.getFreePsram());
#endif

    lv_timer_handler();
    delay(100); // Give some time for the display to initialize
    tft.setBrightness(255);
}

void hal_loop(void)
{
    /* NO while loop in this function! (handled by framework) */
    lv_timer_handler(); // Update the UI-
    delay(5);
}
