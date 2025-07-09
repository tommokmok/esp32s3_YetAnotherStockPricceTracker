#include "storage.h"
#include <string.h>
#include <stdio.h>
#include "nvs_flash.h"
#include "nvs.h"

static nvs_handle_t storage_handle;

void storage_init(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        nvs_flash_erase();
        nvs_flash_init();
    }
    nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &storage_handle);
}

int storage_save(const char *key, const void *data, uint32_t len)
{
    if (!key || !data || len == 0)
        return -1;
    esp_err_t err = nvs_set_blob(storage_handle, key, data, len);
    if (err != ESP_OK)
        return -1;
    err = nvs_commit(storage_handle);
    return (err == ESP_OK) ? 0 : -1;
}

int storage_load(const char *key, void *data, uint32_t len)
{
    if (!key || !data || len == 0)
        return -1;
    size_t required_size = 0;
    esp_err_t err = nvs_get_blob(storage_handle, key, NULL, &required_size);
    if (err != ESP_OK || required_size > len)
        return -1;
    err = nvs_get_blob(storage_handle, key, data, &required_size);
    return (err == ESP_OK) ? required_size : -1;
}

// Add this function to reset storage and restart ESP32
void storage_reset(void)
{
    nvs_flash_erase();
}