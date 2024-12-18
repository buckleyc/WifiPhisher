#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include "vendors.h"


const char *vendor_string[] = 
{
    "Generic\0",
    "Vodafone\0",
    "Fastweb\0",
    "TP-Link\0",
    "TIM\0",
    "D-Link\0",
    "Skywifi\0",
    "Wind\0",
    "Linkem\0",
    "Huawei\0",
    "Netgear\0",
    "Tiscali\0",
    "FritzBox\0",
};


static void to_lowercase(char *input, char *output) {
    for (int i = 0; i < strlen(input); i++) {
        output[i] = tolower(input[i]);
    }
    output[strlen(input)] = '\0';
}


vendors_t getVendor(char *ssid)
{
    if( ssid == NULL )
    {
        return GENERIC;
    }

    char ssid_lower[33] = { 0 };
    char temp_lower[40] = { 0 };
    to_lowercase((char *)ssid, (char *)&ssid_lower);
    printf("SSID: %s\n", ssid_lower);
    for(uint8_t i = 0; i < sizeof(vendor_string)/sizeof(vendor_string[0]); i++ )
    {
        to_lowercase((char *)vendor_string[i], (char *)&temp_lower);
        printf("match: %s\n", temp_lower);
        if( strstr((char *)&ssid_lower, (char *)&temp_lower) != NULL )
        {
            return (vendors_t)i;
        }
    }
    return GENERIC;
}


const char * vendorToString(vendors_t vendor)
{
    return vendor_string[vendor];
}