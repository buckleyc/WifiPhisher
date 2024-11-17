#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "config.h"
#include "wifiMng.h"


static const char *TAG = "WIFI_MNG";


static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED) 
    {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" join, AID=%d", MAC2STR(event->mac), event->aid);
    } 
    else if (event_id == WIFI_EVENT_AP_STADISCONNECTED)
    {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" leave, AID=%d, reason=NULL", MAC2STR(event->mac), event->aid);
    }
}


void wifi_init(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));
}


void wifi_start_softap(void)
{
    wifi_config_t wifi_config = {
        .ap = {
            .ssid = DEFAULT_WIFI_SSID,
            .ssid_len = strlen(DEFAULT_WIFI_SSID),
            .channel = DEFAULT_WIFI_CHAN,
            .password = DEFAULT_WIFI_PASS,
            .max_connection = DEFAULT_WIFI_MAX_CONN,
            .authmode = DEFAULT_WIFI_AUTH,
            .pmf_cfg = {
                    /* Cannot set pmf to required when in wpa-wpa2 mixed mode! Setting pmf to optional mode. */
                    .required = false, 
            }
        }
    };

    if(read_string_from_flash("wifi_ssid", (char *)&wifi_config.ap.ssid) != ESP_OK )
    {
        strcpy((char *)&wifi_config.ap.ssid, DEFAULT_WIFI_SSID);
    }
    if(read_string_from_flash("wifi_pass", (char *)&wifi_config.ap.password) != ESP_OK )
    {
        strcpy((char *)&wifi_config.ap.password, DEFAULT_WIFI_PASS);
    }
    if(read_int_from_flash("wifi_pass", (int32_t *)&wifi_config.ap.channel) != ESP_OK )
    {
        wifi_config.ap.channel = DEFAULT_WIFI_CHAN;
    }


    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));
}


