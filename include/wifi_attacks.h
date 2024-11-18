#ifndef _WIFI_ATTACKS_H
#define _WIFI_ATTACKS_H

#include "esp_system.h"

/**
 * @brief Number of max client to store
 * 
 */
#define MAX_CLIENTS 30

/**
 * @brief List of associated client to the target AP
 * 
 */
typedef struct {
    uint8_t mac[6];
} client_t;


/**
 * @brief Initialize the attack engine
 * 
 */
void wifi_attack_engine_start(uint8_t *bssid);


/**
 * @brief Deauth attack by sending deauth frames and new technique 
 * 
 * @param bssid 
 */
void wifi_attack_deauth(uint8_t *bssid);

#endif