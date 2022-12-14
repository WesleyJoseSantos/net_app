/**
 * @file net_app.h
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2022-09-30
 *
 * @copyright Copyright (c) 2022
 *
 */

#ifndef __NET_APP__H__
#define __NET_APP__H__

#include "esp_wifi.h"
#include "mqtt_client.h"
#include "esp_http_server.h"

/**
 * @brief Network Message ID
 *
 */
typedef enum net_app_msg_id
{
    NET_APP_MSG_ID_START_HTTP_SERVER, ///!< Request to start http server
    NET_APP_MSG_ID_START_WIFI_AP,     ///!< Request to start wifi ap
    NET_APP_MSG_ID_START_WIFI_STA,    ///!< Request to start wifi sta
    NET_APP_MSG_ID_START_MQTT,        ///!< Request to start mqtt
    NET_APP_MSG_ID_SET_SETTINGS,      ///!< Request to set network settings
    NET_APP_MSG_ID_SAVE_SETTINGS,     ///!< Request to save network settings
    NET_APP_MSG_ID_LOAD_SETTINGS,     ///!< Request to load network settings
} net_app_msg_id_t;

/**
 * @brief WiFi connection
 *
 */
typedef struct net_app_wifi_conn
{
    char ip[16];   ///!< WiFi Connection IP
    bool status;   ///!< WiFi Conncetion Status
    char ssid[32]; ///!< WiFi Connection SSID
} net_app_wifi_conn_t;

/**
 * @brief Network app settings
 *
 */
typedef struct net_app_settings
{
    wifi_sta_config_t wifi_sta;    ///!< WiFi station configuration
    esp_mqtt_client_config_t mqtt; ///!< MQTT client configuration
} net_app_settings_t;

/**
 * @brief Network app msg data
 *
 */
typedef union net_app_msg_data
{
    httpd_config_t http_server;     ///!< HTTP server configuration
    wifi_ap_config_t wifi_ap;       ///!< WiFi AP configuration
    wifi_sta_config_t wifi_sta;     ///!< WiFi station configuration
    esp_mqtt_client_config_t mqtt;  ///!< MQTT client configuration
    net_app_settings_t settings;    ///!< Network app settings
} net_app_msg_data_t;

/**
 * @brief Network app queue message
 *
 */
typedef struct net_app_queue_msg
{
    net_app_msg_id_t id;            ///!< Message ID
    net_app_msg_data_t data;        ///!< Message data
} net_app_queue_msg_t;

/**
 * @brief Starts the network RTOS task
 *
 */
void net_app_start(void);

/**
 * @brief Sends a message to network queue
 *
 * @param msg msg to be sended to queue
 * @return BaseType_t pvTRUE if an intem was sucessfully added to the queue otherwise pdFALSE
 */
BaseType_t net_app_send_msg(net_app_queue_msg_t *msg);

/**
 * @brief Waits for message processing
 *
 * @param timeout timeout in ms
 */
void net_app_wait_msg_processing(int timeout);

/**
 * @brief Get WiFi interface connection information
 *
 * @param wifi_interface WiFi interface
 * @param wifi_conn Buffer to store wifi connection information
 */
void net_app_wifi_get_conn(wifi_interface_t wifi_interface, net_app_wifi_conn_t *wifi_conn);

/**
 * @brief Get MQTT connection status
 *
 * @return true if connected
 * @return false if not
 */
bool net_app_mqtt_connected();

/**
 * @brief Get MQTT Client
 *
 * @param mqtt_client mqtt client intance pointer output
 */
void net_app_get_mqtt_client(esp_mqtt_client_handle_t *mqtt_client);

#endif //!__NET_APP__H__