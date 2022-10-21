/**
 * @file dtos.h
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2022-10-01
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#ifndef __DTOS__H__
#define __DTOS__H__

#include "esp_wifi.h"
#include "net_app.h"
#include "mqtt_client.h"

void wifi_sta_config_from_json(wifi_sta_config_t *data, char *json_str);
void wifi_sta_config_to_json(char *json_str, wifi_sta_config_t *data);

void net_app_wifi_sta_info_to_json(char *json_str, net_app_wifi_sta_info_t *data);

void esp_mqtt_client_config_from_json(esp_mqtt_client_config_t *data, char *json_str);
void esp_mqtt_client_config_to_json(char *json_str, esp_mqtt_client_config_t *data);

void net_app_settings_from_json(net_app_settings_t *data, char *json_str);
void net_app_settings_to_json(char *json_str, net_app_settings_t *data);

#endif  //!__DTOS__H__