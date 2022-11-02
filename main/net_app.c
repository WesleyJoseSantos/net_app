/**
 * @file net_app.c
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2022-09-30
 *
 * @copyright Copyright (c) 2022
 *
 */

#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "net_app.h"
#include "http_server.h"
#include "tasks_common.h"
#include "dtos.h"

#define SETTINGS_FILENAME "/web/net_settings.json"
#define EVT_TIMEOUT 15000
#define BIT_WIFI_CONNECTED BIT0
#define BIT_MQTT_CONNECTED BIT1
#define BIT_NTP_SYNC_OK BIT2
#define BIT_MSG_PROCESSING_DONE BIT2

typedef struct wifi_ap_data
{
    bool started;
    esp_netif_t *netif;
    net_app_wifi_ap_info_t info;
} wifi_ap_data_t;

typedef struct wifi_sta_data
{
    bool started;
    esp_netif_t *netif;
    net_app_wifi_sta_info_t info;
} wifi_sta_data_t;

typedef struct wifi
{
    wifi_ap_data_t ap;
    wifi_sta_data_t sta;
} wifi_t;

typedef struct mqtt
{
    bool status;
    esp_mqtt_client_handle_t client;
} mqtt_t;

typedef struct net
{
    TaskHandle_t task;
    EventGroupHandle_t event_group;
    QueueHandle_t queue;
    wifi_t wifi;
    mqtt_t mqtt;
    net_app_settings_t settings;
    bool ntp_sync_ok;
    char id[32];
} net_t;

static const char *TAG = "net_app";

static net_t this;

static void net_app_wifi_init(void);
static void net_app_task(void *pvParameter);
static void net_app_http_server_start(httpd_config_t *cfg);
static void net_app_set_ip_config(net_app_netif_ip_config_t *cfg);
static void net_app_set_netif_ip_config(esp_netif_t *netif, net_app_ip_config_t *cfg);
static void net_app_wifi_ap_start(wifi_ap_config_t *cfg);
static void net_app_wifi_sta_start(wifi_sta_config_t *cfg);
static void net_app_ntp_start(net_app_ntp_config_t *cfg);
static void net_app_mqtt_start(esp_mqtt_client_config_t *cfg);
static void net_app_settings_save(net_app_settings_t *settings);
static void net_app_settings_load(void);
static void net_app_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
static int net_app_mqtt_event_handler(esp_mqtt_event_handle_t event);
static void net_app_ntp_sync_notification_cb();

void net_app_start()
{
    ESP_LOGI(TAG, "Network Application Start");
    esp_log_level_set("wifi", ESP_LOG_NONE);
    this.queue = xQueueCreate(5, sizeof(net_app_queue_msg_t));
    xTaskCreatePinnedToCore(net_app_task, "net_app_task", NET_APP_TASK_STACK_SIZE, NULL, NET_APP_TASK_PRIORITY, this.task, NET_APP_TASK_CORE_ID);
}

BaseType_t net_app_send_msg(net_app_queue_msg_t *msg)
{
    xEventGroupClearBits(this.event_group, BIT_MSG_PROCESSING_DONE);
    return xQueueSend(this.queue, msg, portMAX_DELAY);
}

void net_app_wait_msg_processing(int timeout)
{
    BaseType_t clear_on_exit = pdFALSE;
    BaseType_t wait_for_all_bits = pdTRUE;
    xEventGroupWaitBits(this.event_group, BIT_MSG_PROCESSING_DONE, clear_on_exit, wait_for_all_bits, timeout / portTICK_PERIOD_MS);
}

void net_app_wifi_get_info(wifi_interface_t wifi_interface, net_app_wifi_info_t *info)
{
    switch (wifi_interface)
    {
    case WIFI_IF_STA:
        esp_wifi_sta_get_ap_info(&this.wifi.sta.info.ap_record);
        *info = (net_app_wifi_info_t)this.wifi.sta.info;
        break;
    case WIFI_IF_AP:
        esp_wifi_ap_get_sta_list(&this.wifi.ap.info.sta_list);
        *info = (net_app_wifi_info_t)this.wifi.ap.info;
        break;
    default:
        break;
    }
}

bool net_app_mqtt_connected()
{
    return this.mqtt.status;
}

bool net_app_ntp_sync_ok()
{
    return this.ntp_sync_ok;
}

esp_mqtt_client_handle_t *net_app_mqtt_client()
{
    return &this.mqtt.client;
}

void net_app_get_ip_config(net_app_netif_t netif, net_app_ip_config_t *cfg)
{
    esp_netif_t *esp_netif;
    esp_netif_dhcp_status_t dhcp_status;

    switch (netif)
    {
    case NET_APP_INTERFACE_WIFI_STA:
        esp_netif = this.wifi.sta.netif;
        break;
    case NET_APP_INTERFACE_WIFI_AP:
        esp_netif = this.wifi.ap.netif;
        break;
    case NET_APP_INTERFACE_ETH:
        /// TODO: get ethernet netif
        return;
        break;
    default:
        break;
    }

    esp_netif_dhcpc_get_status(esp_netif, &dhcp_status);
    esp_netif_get_ip_info(esp_netif, &cfg->ip_info);
    esp_netif_get_dns_info(esp_netif, ESP_NETIF_DNS_MAIN, &cfg->dns_info);
    cfg->dhcp = dhcp_status == ESP_NETIF_DHCP_STARTED;
}

static void net_app_wifi_init(void)
{
    wifi_init_config_t wifi_init_cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_cfg));

    this.wifi.ap.netif = esp_netif_create_default_wifi_ap();
    this.wifi.sta.netif = esp_netif_create_default_wifi_sta();

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &net_app_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, ESP_EVENT_ANY_ID, &net_app_event_handler, NULL));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));
}

static void net_app_task(void *pvParameter)
{
    net_app_queue_msg_t msg;
    this.event_group = xEventGroupCreate();
    net_app_wifi_init();

    for (;;)
    {
        if (xQueueReceive(this.queue, &msg, portMAX_DELAY))
        {
            switch (msg.id)
            {
            case NET_APP_MSG_ID_START_HTTP_SERVER:
                ESP_LOGI(TAG, "NET_APP_MSG_ID_START_HTTP_SERVER");
                net_app_http_server_start(&msg.data.http_server);
                break;
            case NET_APP_MSG_ID_SET_IP_CONFIG:
                ESP_LOGI(TAG, "NET_APP_MSG_ID_SET_IP_CONFIG");
                net_app_set_ip_config(&msg.data.ip);
                break;
            case NET_APP_MSG_ID_START_WIFI_AP:
                ESP_LOGI(TAG, "NET_APP_MSG_ID_START_WIFI_AP");
                net_app_wifi_ap_start(&msg.data.wifi_ap);
                break;
            case NET_APP_MSG_ID_START_WIFI_STA:
                ESP_LOGI(TAG, "NET_APP_MSG_ID_START_WIFI_STA");
                net_app_wifi_sta_start(&msg.data.wifi_sta);
                break;
            case NET_APP_MSG_ID_START_NTP:
                ESP_LOGI(TAG, "NET_APP_MSG_ID_START_NTP");
                net_app_ntp_start(&msg.data.ntp);
                break;
            case NET_APP_MSG_ID_START_MQTT:
                ESP_LOGI(TAG, "NET_APP_MSG_ID_START_MQTT");
                net_app_mqtt_start(&msg.data.mqtt);
                break;
            case NET_APP_MSG_ID_SET_SETTINGS:
                ESP_LOGI(TAG, "NET_APP_MSG_ID_SET_SETTINGS");
                net_app_set_netif_ip_config(this.wifi.sta.netif, &msg.data.settings.ip_cfg[NET_APP_INTERFACE_WIFI_STA]);
                net_app_set_netif_ip_config(this.wifi.ap.netif, &msg.data.settings.ip_cfg[NET_APP_INTERFACE_WIFI_AP]);
                /// TODO: ethernet ip config
                net_app_wifi_sta_start(&msg.data.settings.wifi_sta);
                net_app_ntp_start(&msg.data.settings.ntp);
                net_app_mqtt_start(&msg.data.settings.mqtt);
                break;
            case NET_APP_MSG_ID_SAVE_SETTINGS:
                ESP_LOGI(TAG, "NET_APP_MSG_ID_SAVE_SETTINGS");
                net_app_settings_save(&msg.data.settings);
                break;
            case NET_APP_MSG_ID_LOAD_SETTINGS:
                ESP_LOGI(TAG, "NET_APP_MSG_ID_LOAD_SETTINGS");
                net_app_settings_load();
                break;
            default:
                break;
            }
            xEventGroupSetBits(this.event_group, BIT_MSG_PROCESSING_DONE);
        }
    }
}

static void net_app_http_server_start(httpd_config_t *cfg)
{
    cfg->task_priority = HTTP_SERVER_TASK_PRIORITY;
    cfg->core_id = HTTP_SERVER_TASK_CORE_ID;
    cfg->stack_size = HTTP_SERVER_TASK_STACK_SIZE;
    ESP_ERROR_CHECK(http_server_start(cfg));
}

static void net_app_set_ip_config(net_app_netif_ip_config_t *cfg)
{
    switch (cfg->interface)
    {
    case NET_APP_INTERFACE_WIFI_STA:
        ESP_LOGI(TAG, "NET_APP_INTERFACE_WIFI_STA");
        net_app_set_netif_ip_config(this.wifi.sta.netif, &cfg->config);
        break;
    case NET_APP_INTERFACE_WIFI_AP:
        ESP_LOGI(TAG, "NET_APP_INTERFACE_WIFI_AP");
        net_app_set_netif_ip_config(this.wifi.ap.netif, &cfg->config);
        break;
    case NET_APP_INTERFACE_ETH:
        ESP_LOGI(TAG, "NET_APP_INTERFACE_ETH");
        /// TODO: ethernet ip assignment
        break;
    default:
        break;
    }
}

static void net_app_set_netif_ip_config(esp_netif_t *netif, net_app_ip_config_t *cfg)
{
    if (cfg->dhcp)
    {
        esp_netif_dhcpc_start(netif);
    }
    else
    {
        esp_netif_dhcpc_stop(netif);
        esp_netif_set_ip_info(netif, &cfg->ip_info);
        esp_netif_set_dns_info(netif, ESP_NETIF_DNS_MAIN, &cfg->dns_info);
    }
}

static void net_app_wifi_ap_start(wifi_ap_config_t *cfg)
{
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, (wifi_config_t *)cfg));
    ESP_ERROR_CHECK(esp_wifi_start());
    memcpy(this.wifi.ap.info.ssid, cfg->ssid, sizeof(this.wifi.ap.info.ssid));
}

static void net_app_wifi_sta_start(wifi_sta_config_t *cfg)
{
    xEventGroupClearBits(this.event_group, BIT_WIFI_CONNECTED);
    memcpy(&this.settings.wifi_sta, cfg, sizeof(*cfg));
    if (this.wifi.sta.info.conn.status)
        esp_wifi_disconnect();
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, (wifi_config_t *)cfg));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_LOGI(TAG, "ssid: %s", cfg->ssid);
    ESP_LOGI(TAG, "password: %s", cfg->password);
    if (this.wifi.sta.started)
        esp_wifi_connect();
    xEventGroupWaitBits(this.event_group, BIT_WIFI_CONNECTED, false, true, EVT_TIMEOUT / portTICK_PERIOD_MS);
}

static void net_app_ntp_start(net_app_ntp_config_t *cfg)
{
    xEventGroupClearBits(this.event_group, BIT_NTP_SYNC_OK);
    sntp_stop();

    if (cfg->sync_interval == 0)
    {
        cfg->sync_interval = 3600;
    }

    sntp_set_sync_interval(cfg->sync_interval);
    sntp_setoperatingmode(cfg->op_mode);
    sntp_setservername(0, cfg->server1);

    if (cfg->server2 && cfg->server2[0] != '\0')
    {
        sntp_setservername(1, cfg->server2);
    }

    if (cfg->server3 && cfg->server3[0] != '\0')
    {
        sntp_setservername(2, cfg->server3);
    }

    sntp_set_sync_mode(cfg->sync_mode);
    sntp_set_time_sync_notification_cb(net_app_ntp_sync_notification_cb);
    sntp_init();

    setenv("TZ", "<-03>3", 1);
    tzset();
    xEventGroupWaitBits(this.event_group, BIT_NTP_SYNC_OK, true, true, EVT_TIMEOUT / portTICK_PERIOD_MS);
}

static void net_app_mqtt_start(esp_mqtt_client_config_t *cfg)
{
    xEventGroupClearBits(this.event_group, BIT_MQTT_CONNECTED);
    memcpy(&this.settings.mqtt, cfg, sizeof(*cfg));
    if (this.mqtt.status)
        esp_mqtt_client_disconnect(this.mqtt.client);
    sprintf(this.id, "%u", esp_random());
    cfg->task_prio = MQTT_CLIENT_TASK_PRIORITY;
    cfg->task_stack = MQTT_CLIENT_TASK_STACK_SIZE;
    cfg->event_handle = net_app_mqtt_event_handler;
    this.mqtt.client = esp_mqtt_client_init(cfg);
    ESP_ERROR_CHECK(esp_mqtt_client_start(this.mqtt.client));
    xEventGroupWaitBits(this.event_group, BIT_MQTT_CONNECTED, false, true, EVT_TIMEOUT / portTICK_PERIOD_MS);
}

static void net_app_settings_save(net_app_settings_t *settings)
{
    FILE *fd = fopen(SETTINGS_FILENAME, "w");
    if (fd != NULL)
    {
        char data[512];
        net_app_settings_to_json(data, settings);
        fprintf(fd, data);
    }
    else
    {
        ESP_LOGI(TAG, "Error opening file: " SETTINGS_FILENAME);
    }
    fclose(fd);
}

static void net_app_settings_load(void)
{
    FILE *fd;
    struct stat file_stat;

    if (stat(SETTINGS_FILENAME, &file_stat) != -1)
    {
        net_app_queue_msg_t msg;
        memset(&msg, 0, sizeof(msg));
        fd = fopen(SETTINGS_FILENAME, "r");
        if (fd != NULL)
        {
            char json[512];
            msg.id = NET_APP_MSG_ID_SET_SETTINGS;
            fseek(fd, 0L, SEEK_END);
            int size = ftell(fd);
            rewind(fd);
            fread(json, sizeof(char), size, fd);
            json[size] = '\0';
            net_app_settings_from_json(&msg.data.settings, json);
            net_app_send_msg(&msg);
        }
    }
    else
    {
        ESP_LOGI(TAG, SETTINGS_FILENAME " file not exists");
    }
}

static void net_app_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT)
    {
        switch (event_id)
        {
        case WIFI_EVENT_AP_START:
            ESP_LOGI(TAG, "WIFI_EVENT_AP_START");
            this.wifi.ap.started = true;
            break;
        case WIFI_EVENT_AP_STOP:
            ESP_LOGI(TAG, "WIFI_EVENT_AP_STOP");
            this.wifi.ap.started = false;
            break;
        case WIFI_EVENT_AP_STACONNECTED:
            ESP_LOGI(TAG, "WIFI_EVENT_AP_STACONNECTED");
            this.wifi.ap.info.conn.status = true;
            break;
        case WIFI_EVENT_AP_STADISCONNECTED:
            ESP_LOGI(TAG, "WIFI_EVENT_AP_STADISCONNECTED");
            this.wifi.ap.info.conn.status = false;
            break;
        case WIFI_EVENT_STA_START:
            ESP_LOGI(TAG, "WIFI_EVENT_STA_START");
            esp_wifi_connect();
            this.wifi.sta.started = true;
            break;
        case WIFI_EVENT_STA_STOP:
            ESP_LOGI(TAG, "WIFI_EVENT_STA_STOP");
            this.wifi.sta.started = false;
            break;
        case WIFI_EVENT_STA_CONNECTED:
            ESP_LOGI(TAG, "WIFI_EVENT_STA_CONNECTED");
            this.wifi.sta.info.conn.status = true;
            break;
        case WIFI_EVENT_STA_DISCONNECTED:
            ESP_LOGI(TAG, "WIFI_EVENT_STA_DISCONNECTED");
            this.wifi.sta.info.conn.status = false;
            xEventGroupClearBits(this.event_group, BIT_WIFI_CONNECTED);
            break;
        default:
            break;
        }
    }
    else if (event_base == IP_EVENT)
    {
        switch (event_id)
        {
        case IP_EVENT_STA_GOT_IP:
        {
            ESP_LOGI(TAG, "IP_EVENT_STA_GOT_IP");
            ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
            sprintf(this.wifi.sta.info.conn.ip, IPSTR, IP2STR(&event->ip_info.ip));
            xEventGroupSetBits(this.event_group, BIT_WIFI_CONNECTED);
            break;
        }
        case IP_EVENT_AP_STAIPASSIGNED:
        {
            ESP_LOGI(TAG, "IP_EVENT_AP_STAIPASSIGNED");
            ip_event_ap_staipassigned_t *event = (ip_event_ap_staipassigned_t *)event_data;
            sprintf(this.wifi.ap.info.conn.ip, IPSTR, IP2STR(&event->ip));
            break;
        }
        default:
            break;
        }
    }
}

static int net_app_mqtt_event_handler(esp_mqtt_event_handle_t event)
{
    switch (event->event_id)
    {
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        break;
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        this.mqtt.status = true;
        xEventGroupSetBits(this.event_group, BIT_MQTT_CONNECTED);
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        xEventGroupClearBits(this.event_group, BIT_MQTT_CONNECTED);
        break;
    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGD(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
    return ESP_OK;
}

static void net_app_ntp_sync_notification_cb()
{
    this.ntp_sync_ok = true;
    xEventGroupSetBits(this.event_group, BIT_NTP_SYNC_OK);
}
