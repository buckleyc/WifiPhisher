#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include "vendors.h"


static void to_lowercase(char *input, char *output) {
    while (*input) {
        *output = tolower(*input); // Converte il carattere corrente in minuscolo
        input++; // Passa al carattere successivo
    }
}


vendors_t getVendor(uint8_t *ssid)
{
    char ssid_lower[33] = { 0 };
    to_lowercase((char *)ssid, (char *)&ssid_lower);

    if( strstr((char *)&ssid_lower, "vodafone") != NULL )
    {
        return VODAFONE;
    }
    else if( strstr((char *)&ssid_lower, "fastweb") != NULL )
    {
        return FASTWEB;
    }
    else if( strstr((char *)&ssid_lower, "tim") != NULL )
    {
        return TIM;
    }

    return GENERIC;
}