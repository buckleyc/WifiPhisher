#ifndef _VENDORS_H
#define _VENDORS_H


#include "esp_system.h"

typedef enum {
    GENERIC = 0x00,
    VODAFONE,
    FASTWEB,
    TPLINK,
    TIM,
    DLINK,
    SKYWIFI,
    WIND,
    LINKEM,
    HUAWEI,
    NETGER,
    TISCALI,
    FRITZBOX
} vendors_t;


/**
 * @brief Get vendor from ssid
 * 
 * @param ssid 
 * @return vendors_t 
 */
vendors_t getVendor(uint8_t *ssid);


#endif
