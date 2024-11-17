#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "spi_flash_mmap.h"

#include "esp_wifi.h"
#include "esp_event.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include <lwip/sockets.h>
#include <lwip/sys.h>
#include <lwip/api.h>
#include <lwip/netdb.h>

#include "esp_http_server.h"

#include "web_page.h"
#include "server.h"
#include "admin_page.h"
#include "config.h"


static esp_err_t admin_page_handler(httpd_req_t *req) 
{
    char ssid[32] = {0};
    char password[64] = {0};
    int32_t channel = 1;
    char buffer[512] = { 0 };

    /* Read value from flash */
    if( read_string_from_flash("wifi_ssid", ssid) != ESP_OK )
    {
        strcpy(ssid, DEFAULT_WIFI_SSID);
    }
    if( read_string_from_flash("wifi_pass", password) != ESP_OK )
    {
        strcpy(password, DEFAULT_WIFI_PASS);
    }
    if( read_int_from_flash("wifi_chan", channel) != ESP_OK )
    {
        channel = DEFAULT_WIFI_CHAN;
    }
    

    httpd_resp_set_type(req, "text/html");
    /* Send page header */
    httpd_resp_send_chunk(req, admin_page_header, HTTPD_RESP_USE_STRLEN);

    /* Send parameters */
    httpd_resp_send_chunk(req, "<script>", HTTPD_RESP_USE_STRLEN);
    snprintf(buffer, sizeof(buffer), admin_page_settings, ssid, password, (int)channel);
    httpd_resp_send_chunk(req, buffer, HTTPD_RESP_USE_STRLEN);
    httpd_resp_send_chunk(req, "</script>\n", HTTPD_RESP_USE_STRLEN);

    /* Send body */
    httpd_resp_send_chunk(req, admin_page_body, HTTPD_RESP_USE_STRLEN);
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}


static esp_err_t admin_page_settings_ap_submit_handler(httpd_req_t *req)
{
    char buffer[512];
    int ret;

    ret = httpd_req_recv(req, buffer, sizeof(buffer) - 1);
    if (ret <= 0) {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
            httpd_resp_send_408(req);
        }
        return ESP_FAIL;
    }

    buffer[ret] = '\0';
    printf("Dati ricevuti: %s\n", buffer);
    char ssid[64] = {0};
    char password[64] = {0};
    char channel[8] = {0};
    sscanf(buffer, "ssid=%63[^&]&password=%63[^&]&channel=%7s", ssid, password, channel);
    printf("SSID: %s, Password: %s, Channel: %s\n", ssid, password, channel);
    httpd_resp_send(req, "Settings updated successfully!", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
} 


void http_admin_server_start(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
	config.max_open_sockets = 10;
	config.lru_purge_enable = true;
    config.server_port = 8080;
    httpd_handle_t server = NULL;

    if (httpd_start(&server, &config) == ESP_OK) 
	{
        /* Main page handler*/
        httpd_uri_t admin_uri = {
            .uri = "/",
            .method = HTTP_GET,
            .handler = admin_page_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &admin_uri);

        /* Handler for saving admin ap settings */
        httpd_uri_t save_ap_uri = {
            .uri = "/save_ap",
            .method = HTTP_POST,
            .handler = admin_page_settings_ap_submit_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &save_ap_uri);
    }
}