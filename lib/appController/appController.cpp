#include "appController.h"
#include <Arduino.h>
#include <stock_price_gui.h>
#include <storage.h>
#include "lvgl.h"
#include <WiFi.h>

// get stock price from mweb
#include <HTTPClient.h>
#include "WiFiClientSecure.h"
#include <ArduinoJson.h>
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include <WiFiClientSecure.h>

const char *rootCACertificate =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIDxTCCAq2gAwIBAgIQAqxcJmoLQJuPC3nyrkYldzANBgkqhkiG9w0BAQUFADBs\n"
    "MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\n"
    "d3cuZGlnaWNlcnQuY29tMSswKQYDVQQDEyJEaWdpQ2VydCBIaWdoIEFzc3VyYW5j\n"
    "ZSBFViBSb290IENBMB4XDTA2MTExMDAwMDAwMFoXDTMxMTExMDAwMDAwMFowbDEL\n"
    "MAkGA1UEBhMCVVMxFTATBgNVBAoTDERpZ2lDZXJ0IEluYzEZMBcGA1UECxMQd3d3\n"
    "LmRpZ2ljZXJ0LmNvbTErMCkGA1UEAxMiRGlnaUNlcnQgSGlnaCBBc3N1cmFuY2Ug\n"
    "RVYgUm9vdCBDQTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAMbM5XPm\n"
    "+9S75S0tMqbf5YE/yc0lSbZxKsPVlDRnogocsF9ppkCxxLeyj9CYpKlBWTrT3JTW\n"
    "PNt0OKRKzE0lgvdKpVMSOO7zSW1xkX5jtqumX8OkhPhPYlG++MXs2ziS4wblCJEM\n"
    "xChBVfvLWokVfnHoNb9Ncgk9vjo4UFt3MRuNs8ckRZqnrG0AFFoEt7oT61EKmEFB\n"
    "Ik5lYYeBQVCmeVyJ3hlKV9Uu5l0cUyx+mM0aBhakaHPQNAQTXKFx01p8VdteZOE3\n"
    "hzBWBOURtCmAEvF5OYiiAhF8J2a3iLd48soKqDirCmTCv2ZdlYTBoSUeh10aUAsg\n"
    "EsxBu24LUTi4S8sCAwEAAaNjMGEwDgYDVR0PAQH/BAQDAgGGMA8GA1UdEwEB/wQF\n"
    "MAMBAf8wHQYDVR0OBBYEFLE+w2kD+L9HAdSYJhoIAu9jZCvDMB8GA1UdIwQYMBaA\n"
    "FLE+w2kD+L9HAdSYJhoIAu9jZCvDMA0GCSqGSIb3DQEBBQUAA4IBAQAcGgaX3Nec\n"
    "nzyIZgYIVyHbIUf4KmeqvxgydkAQV8GK83rZEWWONfqe/EW1ntlMMUu4kehDLI6z\n"
    "eM7b41N5cdblIZQB2lWHmiRk9opmzN6cN82oNLFpmyPInngiK3BD41VHMWEZ71jF\n"
    "hS9OMPagMRYjyOfiZRYzy78aG6A9+MpeizGLYAiJLQwGXFK3xPkKmNEVX58Svnw2\n"
    "Yzi9RKR/5CYrCsSXaQ3pjOLAEFe4yHYSkVXySGnYvCoCWw9E1CAx2/S6cCZdkGCe\n"
    "vEsXCS+0yx5DaMkHJ8HSXPfqIbloEpw8nL+e/IBcm2PN7EeqJSdnoDfzAIJ9VNep\n"
    "+OkuE6N36B9K\n"
    "-----END CERTIFICATE-----\n";

TaskHandle_t httpTaskHandle = NULL;
// Add your includes here
// UI event
static char saved_ssid[64] = {0};
static char saved_pass[64] = {0};
static uint8_t _get_stock_evt = 0;
static HTTPClient s_http;
WiFiClientSecure *httpsClient = new WiFiClientSecure;

static TimerHandle_t http_timer = NULL;

// Forward declarations
String get_stock_price_yahoo(const char *symbol);
void get_stock_price_all_yahoo(void);
void connect_to_wifi_async(const char *ssid, const char *password);

static uint8_t _gui_hide_loading = 0;
static uint8_t _gui_show_loading = 0;
static uint8_t _gui_update_price = 0;
static uint8_t _gui_update_http_status = 0;
static uint8_t _gui_show_msg_box = 0;
static uint8_t _show_btn_msg_box = 0;
static uint8_t _sw_reset=0;

static String _http_status = " ";
static String _msg_box_content = " ";

static bool wifi_connecting = false;
static unsigned long wifi_connect_start = 0;
static const unsigned long WIFI_CONNECT_TIMEOUT = 5000; // 5seconds


/**
 * Fucntion overide for the GUI implmentation
 */
wifi_ap_info_t load_ap_settings(void)
{
    /**
     * This function should load the saved SSID and password from persistent storage.
     */
    int ret = -1;
    strcpy(saved_ssid, "testssid"); // Replace with actual loading logic
    strcpy(saved_pass, "testpass"); // Replace with actual loading logic

    ret = storage_load(KEY_SSID, (char *)saved_ssid, sizeof(saved_ssid));
    if (ret < 0)
    {
        LV_LOG_ERROR("Failed to load SSID from storage");
    }

    ret = storage_load(KEY_PASSWORD, (char *)saved_pass, sizeof(saved_pass));
    if (ret < 0)
    {
        LV_LOG_ERROR("Failed to load PASS from storage");
    }

    wifi_ap_info_t data = {
        .ssid = saved_ssid,
        .pass = saved_pass};

    return data;
}

/// @brief Function override for saving the AP settings
/// This function should save the SSID and password to persistent storage.
/// @param ssid  It is a pointer to the local static variable of char array with size 64
/// @param pass  It is a pointer to the local static variable of char array with size 64
void save_ap_settings(const char *ssid, const char *pass)
{
    int ret = -1;
    ret = storage_save(KEY_SSID, ssid, 64);
    if (ret < 0)
    {
        LV_LOG_ERROR("Failed to save SSID to storage");
    }

    ret = storage_save(KEY_PASSWORD, pass, 64);
    if (ret < 0)
    {
        LV_LOG_ERROR("Failed to save PASS to storage");
    }

    LV_LOG_USER("Save ret=%d,Saved SSID: %s, Password: %s", ret, ssid, pass);
    // Show message box
    _gui_show_msg_box = 1;
    _msg_box_content = "Saved SSID: " + String(ssid) + ", Password: " + String(pass);

    //Update the lcoal ssid and pass
    strncpy(saved_ssid, ssid, sizeof(saved_ssid) - 1);
    strncpy(saved_pass, pass, sizeof(saved_pass) - 1);

    //Try to connect the wifi
    connect_to_wifi_async(saved_ssid, saved_pass);


}

void update_btn_event_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    LV_LOG_USER("HTTP request");
    xTaskNotifyGive(httpTaskHandle);
}

void msg_box_ok_event_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    Serial.println("Msg box OK button clicked");
    storage_reset();
    _sw_reset=1;


}
void sw_reset(lv_event_t *e)
{
    LV_UNUSED(e);
    //Reset the nvs and software restart the device
    _msg_box_content="All SSID settings will be deleted.\nClick Ok to continue.\nDevice will be restarted.";
    _show_btn_msg_box = 1;

}

void http_timeout_callback(TimerHandle_t xTimer)
{
    if (s_http.connected())
    {
        Serial.println("Force timeout for HTTP request");
        s_http.end();
        _gui_hide_loading = 1;
        // _msg_box_content="Fail to connect the server";
        // show_msg_box("Error", "Fail to connect the server");
    }
}
void start_http_timeout_timer(uint32_t timeout_ms)
{

    if (http_timer == NULL)
    {
        http_timer = xTimerCreate("HttpTimeout", pdMS_TO_TICKS(timeout_ms), pdFALSE, NULL, http_timeout_callback);
    }
    xTimerStop(http_timer, 0);
    xTimerChangePeriod(http_timer, pdMS_TO_TICKS(timeout_ms), 0);
    xTimerStart(http_timer, 0);
}

void stop_http_timeout_timer()
{
    if (http_timer)
    {
        xTimerStop(http_timer, 0);
    }
}


void connect_to_wifi_async(const char *ssid, const char *password)
{
    if (!wifi_connecting)
    {
        WiFi.mode(WIFI_STA);
        WiFi.begin(ssid, password);
        Serial.print("Connecting to WiFi SSID: ");
        Serial.println(ssid);
        wifi_connect_start = millis();
        wifi_connecting = true;
    }
}

/// @brief Function to handle WiFi connection status
/// This function runs in the main loop
/// So, it is ok directly call the gui functions
void handle_wifi_connect()
{
    if (!wifi_connecting)
        return;

    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.println("\nWiFi connected!");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
        // show_msg_box("WiFi Status", "Successfully connected to WiFi!");
        gui_update_wifi_status("Connected");
        wifi_connecting = false;
    }
    else if (millis() - wifi_connect_start > WIFI_CONNECT_TIMEOUT)
    {
        Serial.println("\nWiFi connect timeout.");
        WiFi.disconnect();
        wifi_connecting = false;
        show_msg_box("Error", "Wifi connect timeout!");
    }
}

void http_get_all_stock_prices(void)
{
    LV_LOG_USER("HTTP GET all stock prices");
    _gui_show_loading = 1;
    get_stock_price_all_yahoo();
    _gui_hide_loading = 1;
    _gui_update_price = 1;

}

void https_get_all_stock_prices(void)
{
    String sprice = "N/A"; // Default value if fetching fails
    char (*symbols)[5] = gui_get_symbol_list();
    char (*prices)[10] = gui_get_price_list();
    if (WiFi.status() == WL_CONNECTED)
    {
        httpsClient->setCACert(rootCACertificate); // Set the root CA certificate
        httpsClient->setHandshakeTimeout(8000);    // Set handshake timeout
        // httpsClient->setInsecure();                // Disable certificate validation for testing purposes (not recommended for production)
        for (int i = 0; i < 4; ++i)
        {

            String symbol = String(symbols[i]); // Adjust based on your actual data structure
            Serial.printf("Fetching price for symbol: %s\n", symbol.c_str());
            _http_status = "Fetching " + symbol + "... ";
            _gui_update_http_status = 1;

            String url = String("https://query1.finance.yahoo.com/v8/finance/chart/") + symbol + "?interval=1d";
            Serial.println(url.c_str());
            s_http.begin(*httpsClient, url);
            s_http.setUserAgent("EchoapiRuntime/1.1.0");
            s_http.setTimeout(5000);
            // Start external timeout timer
            // start_http_timeout_timer(10000);

            int httpCode = s_http.GET();
            stop_http_timeout_timer();
            if (httpCode == 200)
            {

                String payload = s_http.getString();
                // Serial.println("Response: " + payload);

                // Parse JSON
                JsonDocument doc;
                DeserializationError error = deserializeJson(doc, payload);
                if (!error)
                {
                    float price = doc["chart"]["result"][0]["meta"]["regularMarketPrice"];
                    sprice = "$" + String(price, 2);        // Format price to 2 decimal places
                    strncpy(prices[i], sprice.c_str(), 10); // Store in prices array
                }
                else
                {
                    Serial.println("Failed to parse JSON.");
                }
            }
            else
            {
                _http_status = "HTTP GET failed, error:" + httpCode;
                _gui_update_http_status = 1;
                // gui_update_loading_status(status.c_str());
                Serial.printf("HTTP GET failed, error: %d\n", httpCode);
            }
        }
        s_http.end();
        Serial.println("Updated stock prices:");
        for (int i = 0; i < 4; ++i)
        {
            Serial.printf("Symbol: %s, Price: %s\n", symbols[i], prices[i]);
        }
    }
    else
    {
        Serial.println("WiFi not connected");
    }
}

void get_stock_price_all_yahoo(void)
{
    String sprice = "N/A"; // Default value if fetching fails
    char (*symbols)[5] = gui_get_symbol_list();
    char (*prices)[10] = gui_get_price_list();
    if (WiFi.status() == WL_CONNECTED)
    {

        for (int i = 0; i < 4; ++i)
        {

            String symbol = String(symbols[i]); // Adjust based on your actual data structure
            Serial.printf("Fetching price for symbol: %s\n", symbol.c_str());
            _http_status = "Fetching " + symbol + "... ";
            _gui_update_http_status = 1;

            String url = String("https://query1.finance.yahoo.com/v8/finance/chart/") + symbol + "?interval=1d";
            Serial.println(url.c_str());
            s_http.begin(url);
            s_http.setUserAgent("EchoapiRuntime/1.1.0");
            s_http.setTimeout(5000);
            // Start external timeout timer
            start_http_timeout_timer(10000);

            int httpCode = s_http.GET();
            stop_http_timeout_timer();
            if (httpCode == 200)
            {

                String payload = s_http.getString();
                // Serial.println("Response: " + payload);

                // Parse JSON
                JsonDocument doc;
                DeserializationError error = deserializeJson(doc, payload);
                if (!error)
                {
                    float price = doc["chart"]["result"][0]["meta"]["regularMarketPrice"];
                    sprice = "$" + String(price, 2);        // Format price to 2 decimal places
                    strncpy(prices[i], sprice.c_str(), 10); // Store in prices array
                }
                else
                {
                    Serial.println("Failed to parse JSON.");
                }
            }
            else
            {
                _http_status = "HTTP GET failed, error:" + httpCode;
                _gui_update_http_status = 1;
                // gui_update_loading_status(status.c_str());
                Serial.printf("HTTP GET failed, error: %d\n", httpCode);
            }
        }
        s_http.end();
        Serial.println("Updated stock prices:");
        for (int i = 0; i < 4; ++i)
        {
            Serial.printf("Symbol: %s, Price: %s\n", symbols[i], prices[i]);
        }
    }
    else
    {
        Serial.println("WiFi not connected");
    }
}

String get_stock_price_yahoo(const char *symbol)
{
    String sprice = "N/A"; // Default value if fetching fails
    if (WiFi.status() == WL_CONNECTED)
    {
        HTTPClient http;
        String url = String("https://query1.finance.yahoo.com/v8/finance/chart/") + symbol + "?interval=1d";
        Serial.println(url.c_str());
        http.begin(url);
        http.setUserAgent("EchoapiRuntime/1.1.0");
        int httpCode = http.GET();

        if (httpCode == 200)
        {
            String payload = http.getString();
            // Serial.println("Response: " + payload);

            // Parse JSON
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, payload);
            if (!error)
            {
                float price = doc["chart"]["result"][0]["meta"]["regularMarketPrice"];
                sprice = "$" + String(price, 2); // Format price to 2 decimal places
            }
            else
            {
                Serial.println("Failed to parse JSON.");
            }
        }
        else
        {
            Serial.printf("HTTP GET failed, error: %d\n", httpCode);
        }
        http.end();
    }
    else
    {
        Serial.println("WiFi not connected");
    }

    return sprice;
}
// Task function for HTTP request
void http_request_task(void *parameter)
{
    while (1)
    {
        // Wait for notification to start HTTP request
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        LV_LOG_USER("Starting HTTP request task");
        // Perform HTTP request
        // get_stock_price_yahoo("AAPL");
        http_get_all_stock_prices();

    }
}

void app_controller_init(void)
{

    // Only connect the wifi if the ssid is not the default
    if (strcmp(saved_ssid, "testssid") != 0)
    {
        connect_to_wifi_async(saved_ssid, saved_pass);
        Serial.print("Connecting to WiFi SSID: ");
        Serial.println(saved_ssid);
    }
    else{
        Serial.println("Default SSID detected, not connecting to WiFi.");
    }

    // Create the HTTP request task
    if (httpTaskHandle == NULL)
    {
        xTaskCreate(
            http_request_task,   // Task function
            "HTTP_Request_Task", // Name
            8192,                // Stack size
            NULL,                // Parameter
            1,                   // Priority
            &httpTaskHandle      // Task handle
        );
    }
}
// Loop that runs on main loop of the Ardunio
void app_controller_run(void)
{
    handle_wifi_connect();
    // flags for the GUI signal
    if (_gui_show_loading)
    {
        gui_show_loading_screen("Loading...");
        _gui_show_loading = 0;
    }
    if (_gui_hide_loading)
    {
        gui_hide_loading_screen();
        _gui_hide_loading = 0;
    }
    if (_gui_update_price)
    {
        gui_update_price();
        _gui_update_price = 0;
    }
    if (_gui_update_http_status)
    {
        gui_update_loading_status(_http_status.c_str());
        _gui_update_http_status = 0;
    }
    if (_gui_show_msg_box)
    {
        show_msg_box("Error", _msg_box_content.c_str());
        _gui_show_msg_box = 0;
    }
    if (_show_btn_msg_box)
    {
        show_btn_msg_box("Factory Reset", _msg_box_content.c_str(), msg_box_ok_event_cb);
        _show_btn_msg_box = 0;
    }
    if (_sw_reset)
    {

        Serial.println("Software reset triggered, restarting device...");
        delay(10);
        esp_restart(); // Restart the ESP32
        while(1);
    }
}