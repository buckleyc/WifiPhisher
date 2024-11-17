#ifndef _EVIL_TWIN_H
#define _EVIL_TWIN_H

#include "esp_wifi.h"
#include "esp_system.h"
#include "vendors.h"

/**
 * @brief Struct containing the current evil twin target
 * 
 */
typedef struct {
    uint8_t bssid[6];
    uint8_t ssid[33]; 
    int8_t rssi;
    uint8_t channel;
    vendors_t vendor;
} target_info_t;


/**
 * @brief Start EVIL TWIN attack, before lauching be sure to fill target struct
 * 
 */
void evil_twin_start_attack(void);


/**
 * @brief Set evil twin target information
 * 
 * @param targe_info 
 */
void evil_twin_set_target(target_info_t *targe_info);


/**
 * @brief Check the user input password
 * 
 * @param password 
 * @return esp_err_t 
 */
esp_err_t evil_twin_check_password(char *password);

#endif