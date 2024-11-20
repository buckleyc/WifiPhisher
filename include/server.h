#ifndef _SERVER_H
#define _SERVER_H

#include "wifi_attacks.h"

/**
 * @brief Start http server task
 * 
 */
void http_attack_server_start(target_info_t *_target_info);


/**
 * @brief Stop http server
 * 
 */
void http_attack_server_stop(void);

#endif