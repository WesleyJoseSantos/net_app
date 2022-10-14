/**
 * @file main.h
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2022-09-30
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#ifndef __MAIN__H__
#define __MAIN__H__

#include <stdio.h>
#include "nvs_flash.h"
#include "esp_spiffs.h"
#include "esp_log.h"

#include "net_app.h"


#define SPIFFS_CFG() {                  \
    .base_path = "/web",                \
    .partition_label = NULL,            \
    .max_files = 5,                     \
    .format_if_mount_failed = true      \
}

#define NET_APP_MSG_START_HTTP_SERVER() {       \
    .id = NET_APP_MSG_ID_START_HTTP_SERVER,     \
    .data = {                                   \
        .http_server = HTTPD_DEFAULT_CONFIG()   \
    }                                           \
}

#define NET_APP_MSG_START_WIFI_AP() {           \
    .id = NET_APP_MSG_ID_START_WIFI_AP,         \
    .data = {                                   \
        .wifi_ap = {                            \
            .ssid = "NET APP AP",               \
            .password = "administrator",        \
            .authmode = WIFI_AUTH_WPA2_PSK,     \
            .max_connection = 4,                \
            .beacon_interval = 100,             \
        }                                       \
    }                                           \
}

#define NET_APP_MSG_LOAD_SETTINGS() {           \
    .id = NET_APP_MSG_ID_LOAD_SETTINGS,         \
}

#endif  //!__MAIN__H__