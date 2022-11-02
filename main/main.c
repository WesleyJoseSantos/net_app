/**
 * @file main.c
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2022-09-30
 * 
 * @copyright Copyright (c) 2022
 * 
 */ 

#include "main.h"

#define TIMEOUT 5000

static const char *TAG = "main";

static esp_err_t esp_nvs_flash_init();

void app_main(void)
{
    ESP_LOGI(TAG, "Main Application Start");

    net_app_queue_msg_t msg_start_wifi_ap = NET_APP_MSG_START_WIFI_AP();
    net_app_queue_msg_t msg_start_http_server = NET_APP_MSG_START_HTTP_SERVER();
    net_app_queue_msg_t msg_load_settings = NET_APP_MSG_LOAD_SETTINGS();
    esp_vfs_spiffs_conf_t spiffs_conf = SPIFFS_CFG();
    
    ESP_ERROR_CHECK(esp_nvs_flash_init);
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(esp_vfs_spiffs_register(&spiffs_conf));
    
    net_app_start();
    net_app_send_msg(&msg_start_http_server);
    net_app_send_msg(&msg_start_wifi_ap);
    net_app_send_msg(&msg_load_settings);
    net_app_wait_msg_processing(TIMEOUT);
}

static esp_err_t esp_nvs_flash_init()
{
    esp_err_t ret = nvs_flash_init();
    if(ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    return ret;
}
