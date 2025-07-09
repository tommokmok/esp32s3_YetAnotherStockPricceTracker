#ifndef STORAGE_H
#define STORAGE_H

#ifdef __cplusplus
extern "C" {
#endif
#define STORAGE_NAMESPACE "appdata"
#define KEY_SSID "ssid"
#define KEY_PASSWORD "password"
// Add your includes here
#include <stdint.h>

// Define constants, macros, or enums here

// Declare your structs and typedefs here

// Function declarations
void storage_init(void);
int storage_save(const char *key, const void *data, uint32_t len);
int storage_load(const char *key, void *data, uint32_t len);
void storage_delete(const char *key);
void storage_reset(void);

#ifdef __cplusplus
}
#endif

#endif // STORAGE_H