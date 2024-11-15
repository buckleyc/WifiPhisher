#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"

#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include <lwip/sockets.h>
#include <lwip/sys.h>
#include <lwip/api.h>
#include <lwip/netdb.h>

#include "web_page.h"
#include "server.h"


static const char *TAG = "HTTPD";	//TAG for debug
static TaskHandle_t httpTaskHandle = NULL;


static void http_server_netconn_serve(struct netconn *conn)
{
	struct netbuf *inbuf;
	char *buf;
	u16_t buflen;
	err_t err;

	/* Read the data from the port, blocking if nothing yet there.
	 We assume the request (the part we care about) is in one netbuf */
	err = netconn_recv(conn, &inbuf);

	if (err == ERR_OK) {
		netbuf_data(inbuf, (void**) &buf, &buflen);
		/* Is this an HTTP GET command? (only check the first 5 chars, since
		 there are other formats for GET, and we're keeping it very simple )*/
		if (buflen >= 5 && strncmp("GET ",buf,4)==0)
        {
			/*  sample:
			 * 	GET /l HTTP/1.1
				Accept: text/html, application/xhtml+xml, image/jxr,
				Referer: http://192.168.1.222/h
				Accept-Language: en-US,en;q=0.8,zh-Hans-CN;q=0.5,zh-Hans;q=0.3
				User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/51.0.2704.79 Safari/537.36 Edge/14.14393
				Accept-Encoding: gzip, deflate
				Host: 192.168.1.222
				Connection: Keep-Alive
			 *
			 */
			//Parse URL
			char* path = NULL;
			char* line_end = strchr(buf, '\n');
			if( line_end != NULL )
			{
				//Extract the path from HTTP GET request
				path = (char*)malloc(sizeof(char)*(line_end-buf+1));
				int path_length = line_end - buf - strlen("GET ")-strlen("HTTP/1.1")-2;
				strncpy(path, &buf[4], path_length );
				path[path_length] = '\0';
				//Get remote IP address
				ip_addr_t remote_ip;
				u16_t remote_port;
				netconn_getaddr(conn, &remote_ip, &remote_port, 0);
				printf("[ "IPSTR" ] GET %s\n", IP2STR(&(remote_ip.u_addr.ip4)),path);
			}

			/* Send the HTML header
			 * subtract 1 from the size, since we dont send the \0 in the string
			 * NETCONN_NOCOPY: our data is const static, so no need to copy it
			 */
			bool bNotFound = false;
			if(path != NULL)
			{
				if (strcmp("/high",path) == 0) {
					//gpio_set_level(LED_PIN,1);
				}
				else if (strcmp("/low",path) == 0) {
					//gpio_set_level(LED_PIN,0);
				}
				else if (strcmp("/",path) == 0)
				{
				
				}
				else
				{
					bNotFound = true;	// 404 Not found
				}
				free(path);
				path = NULL;
			}
			// Send HTTP response header
			if(bNotFound)
			{
				netconn_write(conn, http_404_hdr, sizeof(http_404_hdr) - 1, NETCONN_NOCOPY);
			}
			else
			{
				netconn_write(conn, http_html_hdr, sizeof(http_html_hdr) - 1, NETCONN_NOCOPY);
			}

			netconn_write(conn, http_index_hml, sizeof(http_index_hml) - 1, NETCONN_NOCOPY);
		}
	}
	/* Close the connection (server closes in HTTP) */
	netconn_close(conn);
	netbuf_delete(inbuf);
}


static void http_server(void *pvParameters) 
{
    /* conn is listening thread, newconn is new thread for each client */
	struct netconn *conn, *newconn;
	err_t err;
	conn = netconn_new(NETCONN_TCP);
	netconn_bind(conn, NULL, 80);
	netconn_listen(conn);
	do 
    {
		err = netconn_accept(conn, &newconn);
		if (err == ERR_OK) {
			http_server_netconn_serve(newconn);
			netconn_delete(newconn);
		}
	} while (err == ERR_OK);
	netconn_close(conn);
	netconn_delete(conn);
}


void http_server_start(void)
{
    /* If task is already started kill it */
    if( httpTaskHandle != NULL )
    {
        http_server_stop();
    }
    /* Start task */
    BaseType_t xReturned = xTaskCreate(&http_server, "http_server", 2048, NULL, 5, &httpTaskHandle);
    if( xReturned != pdPASS )
    {
        httpTaskHandle = NULL;
    }
}


void http_server_stop(void)
{
    if( httpTaskHandle != NULL )
    {
        vTaskDelete(httpTaskHandle);
    }
}