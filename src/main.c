#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_task_wdt.h"
#include "nvs_flash.h"
#include "server.h"
#include "admin_server.h"
#include "wifiMng.h"
#include "dns.h"

void app_main() 
{
    /* Initialize NVS */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    /* Config wdt */
    esp_task_wdt_config_t wdt_conf = {
      .idle_core_mask = 0x3,
      .timeout_ms = 10000,
      .trigger_panic = false
    };
    esp_task_wdt_init(&wdt_conf);

    /* Init wifi */
    wifi_init();

    /* Start wifi AP */
    wifi_start_softap();

    /* Start dns server */
    dns_server_start();

    /* Start admin http server */
    http_admin_server_start();
}