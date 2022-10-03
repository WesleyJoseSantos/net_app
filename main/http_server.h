/**
 * @file http_server.h
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2022-09-30
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#ifndef __HTTP_SERVER__H__
#define __HTTP_SERVER__H__

#include "esp_http_server.h"

/**
 * @brief Start http server
 * 
 * @param config Server configuration
 */
int http_server_start(httpd_config_t *config);

/**
 * @brief Stop http server
 * 
 */
int http_server_stop(void);

#endif  //!__HTTP_SERVER__H__