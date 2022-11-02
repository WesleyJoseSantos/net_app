/**
 * @file http_server.c
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2022-09-30
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include <sys/stat.h>

#include "dtos.h"
#include "esp_log.h"
#include "net_app.h"
#include "http_server.h"
#include "tasks_common.h"
#include "cJSON.h"

#define IS_FILE_EXT(filename, ext) (strcasecmp(&filename[strlen(filename) - sizeof(ext) + 1], ext) == 0)
#define HOMEPAGE "/index.html"
#define STATUS_MSG "{\"err\":%d,\"msg\":\"%s\"}"
#define STATUS_MSG_OK "{\"err\":0,\"msg\":\"OK\"}"

#define FILE_SERVER_BUFSIZE 1023
#define WIFI_TIMEOUT 15000
#define NTP_TIMEOUT 8000
#define MQTT_TIMEOUT 5000

/**
 * @brief Http Server Error Codes
 * 
 */
typedef enum http_server_err
{
    HTTP_SERVER_ERR_OK = ESP_OK,                       ///!< No Error
    HTTP_SERVER_ERR_BASE = (ESP_ERR_HTTPD_BASE + 10),  ///!< Starting number of Http Server Error Codes
    HTTP_SERVER_ERR_NOT_EXISTS,                        ///!< Content not exists
} http_server_err_t;

httpd_handle_t this;

static const char *TAG = "http_server";

static void http_server_register_routes(void);

static esp_err_t net_post_handler(httpd_req_t *req);
static esp_err_t net_ipconfig_post_handler(httpd_req_t *req);
static esp_err_t net_ipconfig_get_handler(httpd_req_t *req);
static esp_err_t net_wifi_post_handler(httpd_req_t *req);
static esp_err_t net_wifi_index_handler(httpd_req_t *req);
static esp_err_t net_wifi_ssids_handler(httpd_req_t *req);
static esp_err_t net_ntp_index_handler(httpd_req_t *req);
static esp_err_t net_ntp_post_handler(httpd_req_t *req);
static esp_err_t net_mqtt_post_handler(httpd_req_t *req);
static esp_err_t net_mqtt_index_handler(httpd_req_t *req);
static esp_err_t file_get_handler(httpd_req_t *req);
static esp_err_t set_content_type_from_file(httpd_req_t *req, const char *filename);

static esp_err_t system_reset_index_handler(httpd_req_t *req);

typedef struct file_server
{
    char buf[FILE_SERVER_BUFSIZE];
} file_server_t;

int http_server_start(httpd_config_t *config)
{
    config->uri_match_fn = httpd_uri_match_wildcard;
    config->max_uri_handlers = 15;
    
    int err_code = httpd_start(&this, config);
    if(err_code == ESP_OK)
    {
        ESP_LOGI(TAG, "Server Started on port %d", config->server_port);
        http_server_register_routes();
    }
    return err_code;
}

int http_server_stop(void)
{
    return httpd_stop(this);
}

static void http_server_register_routes(void)
{
    static file_server_t file_server;

    httpd_uri_t net_post = {
        .uri = "/v1/net",
        .handler = net_post_handler,
        .method = HTTP_POST,
    };
    httpd_register_uri_handler(this, &net_post);

    httpd_uri_t net_ipconfig_post = {
        .uri = "/v1/net/ipconfig/*",
        .handler = net_ipconfig_post_handler,
        .method = HTTP_POST,
    };
    httpd_register_uri_handler(this, &net_ipconfig_post);

    httpd_uri_t net_ipconfig_get = {
        .uri = "/v1/net/ipconfig/*",
        .handler = net_ipconfig_get_handler,
        .method = HTTP_GET,
    };
    httpd_register_uri_handler(this, &net_ipconfig_get);

    httpd_uri_t net_wifi_post = {
        .uri = "/v1/net/wifi",
        .handler = net_wifi_post_handler,
        .method = HTTP_POST,
    };
    httpd_register_uri_handler(this, &net_wifi_post);

    httpd_uri_t net_wifi_index = {
        .uri = "/v1/net/wifi",
        .handler = net_wifi_index_handler,
        .method = HTTP_GET,
    };
    httpd_register_uri_handler(this, &net_wifi_index);

    httpd_uri_t net_wifi_ssids_index = {
        .uri = "/v1/net/wifi/ssids",
        .handler = net_wifi_ssids_handler,
        .method = HTTP_GET,
    };
    httpd_register_uri_handler(this, &net_wifi_ssids_index);

    httpd_uri_t net_ntp_post = {
        .uri = "/v1/net/ntp",
        .handler = net_ntp_post_handler,
        .method = HTTP_POST,
    };
    httpd_register_uri_handler(this, &net_ntp_post);

    httpd_uri_t net_ntp_index = {
        .uri = "/v1/net/ntp",
        .handler = net_ntp_index_handler,
        .method = HTTP_GET,
    };
    httpd_register_uri_handler(this, &net_ntp_index);

    httpd_uri_t net_mqtt_post = {
        .uri = "/v1/net/mqtt",
        .handler = net_mqtt_post_handler,
        .method = HTTP_POST,
    };
    httpd_register_uri_handler(this, &net_mqtt_post);

    httpd_uri_t net_mqtt_index = {
        .uri = "/v1/net/mqtt",
        .handler = net_mqtt_index_handler,
        .method = HTTP_GET,
    };
    httpd_register_uri_handler(this, &net_mqtt_index);

    httpd_uri_t system_reset_index = {
        .uri = "/v1/system/reset",
        .handler = system_reset_index_handler,
        .method = HTTP_GET,
    };
    httpd_register_uri_handler(this, &system_reset_index);

    // Must be the last uri register
    httpd_uri_t file_get = {
        .uri = "/*",
        .handler = file_get_handler,
        .method = HTTP_GET,
        .user_ctx = &file_server,
    };
    httpd_register_uri_handler(this, &file_get);
}

static esp_err_t net_post_handler(httpd_req_t *req)
{
    char content[512];
    net_app_queue_msg_t msg;
    httpd_req_recv(req, content, sizeof(content));

    msg.id = NET_APP_MSG_ID_SAVE_SETTINGS;
    net_app_settings_from_json(&msg.data.settings, content);
    net_app_send_msg(&msg);

    httpd_resp_set_status(req, HTTPD_200);
    httpd_resp_set_type(req, HTTPD_TYPE_JSON);
    httpd_resp_send(req, STATUS_MSG_OK, sizeof(STATUS_MSG_OK));

    return ESP_OK;
}

static esp_err_t net_ipconfig_post_handler(httpd_req_t *req)
{
    int id = atoi(&req->uri[strlen(req->uri) - 1]);
    char content[256];

    if(id >= NET_APP_INTERFACE_COUNT)
    {
        int len = sprintf(content, STATUS_MSG, HTTP_SERVER_ERR_NOT_EXISTS, "Specified interface not exists");
        httpd_resp_set_status(req, HTTPD_400);
        httpd_resp_set_type(req, HTTPD_TYPE_JSON);
        httpd_resp_send(req, content, len);
    }
    else
    {
        net_app_queue_msg_t msg;
        httpd_req_recv(req, content, sizeof(content));
        msg.id = NET_APP_MSG_ID_SET_IP_CONFIG;
        msg.data.ip.interface = id;
        net_app_ip_config_from_json(&msg.data.ip.config, content);
        net_app_send_msg(&msg);

        httpd_resp_set_status(req, HTTPD_200);
        httpd_resp_set_type(req, HTTPD_TYPE_JSON);
        httpd_resp_send(req, STATUS_MSG_OK, sizeof(STATUS_MSG_OK));
    }

    return ESP_OK;
}

static esp_err_t net_ipconfig_get_handler(httpd_req_t *req)
{
    int id = atoi(&req->uri[strlen(req->uri) - 1]);
    char content[256];

    if(id >= NET_APP_INTERFACE_COUNT)
    {
        int len = sprintf(content, STATUS_MSG, HTTP_SERVER_ERR_NOT_EXISTS, "Specified interface not exists");
        httpd_resp_set_status(req, HTTPD_400);
        httpd_resp_set_type(req, HTTPD_TYPE_JSON);
        httpd_resp_send(req, content, len);
    }
    else
    {
        net_app_ip_config_t ip_config;
        net_app_get_ip_config(id, &ip_config);
        net_app_ip_config_to_json(content, &ip_config);
        httpd_resp_set_status(req, HTTPD_200);
        httpd_resp_set_type(req, HTTPD_TYPE_JSON);
        httpd_resp_send(req, content, strlen(content));
    }

    return ESP_OK;
}

static esp_err_t net_wifi_post_handler(httpd_req_t *req)
{
    char content[256];
    net_app_queue_msg_t msg;
    net_app_wifi_info_t info;
    httpd_req_recv(req, content, sizeof(content));

    msg.id = NET_APP_MSG_ID_START_WIFI_STA;
    wifi_sta_config_from_json(&msg.data.wifi_sta, content);
    net_app_send_msg(&msg);
    net_app_wait_msg_processing(WIFI_TIMEOUT);

    net_app_wifi_get_info(WIFI_IF_STA, &info);
    net_app_wifi_sta_info_to_json(content, &info.sta);
    char *status = info.sta.conn.status ? HTTPD_200 : HTTPD_408;
    
    httpd_resp_set_status(req, status);
    httpd_resp_set_type(req, HTTPD_TYPE_JSON);
    httpd_resp_sendstr(req, content);

    ESP_LOGI(TAG, "WIFI HTTP POST");

    return ESP_OK;
}

static esp_err_t net_wifi_index_handler(httpd_req_t *req)
{
    char content[256];
    net_app_wifi_info_t info;
    net_app_wifi_get_info(WIFI_IF_STA, &info);
    net_app_wifi_sta_info_to_json(content, &info.sta);

    httpd_resp_set_status(req, HTTPD_200);
    httpd_resp_set_type(req, HTTPD_TYPE_JSON);
    httpd_resp_sendstr(req, content);

    return ESP_OK;
}

static esp_err_t net_wifi_ssids_handler(httpd_req_t *req)
{
    static esp_err_t err;
    char content[512];
    uint16_t list_siz = 10;
    wifi_ap_record_t wifi_ap_record[list_siz];
    memset(&wifi_ap_record, 0, sizeof(wifi_ap_record_t) * list_siz);

    err = esp_wifi_scan_start(NULL, true);
    if(err == ESP_OK) err = esp_wifi_scan_get_ap_records(&list_siz, wifi_ap_record);

    if(err == ESP_OK)
    {
        wifi_ap_record_to_json(content, wifi_ap_record, list_siz);
        httpd_resp_set_type(req, HTTPD_TYPE_JSON);
        httpd_resp_set_status(req, HTTPD_200);
        httpd_resp_sendstr(req, content);
        return ESP_OK;
    }
    else
    {
        sprintf(content, STATUS_MSG, err, esp_err_to_name(err));
        httpd_resp_set_type(req, HTTPD_TYPE_JSON);
        httpd_resp_set_status(req, HTTPD_500);
        httpd_resp_sendstr(req, content);
        return ESP_FAIL;
    }
}

static esp_err_t net_ntp_post_handler(httpd_req_t *req)
{
    char content[256];
    net_app_queue_msg_t msg;
    httpd_req_recv(req, content, sizeof(content));

    msg.id = NET_APP_MSG_ID_START_NTP;
    net_app_ntp_config_from_json(&msg.data.ntp, content);
    net_app_send_msg(&msg);
    net_app_wait_msg_processing(NTP_TIMEOUT);

    char *status = net_app_ntp_sync_ok() ? HTTPD_200 : HTTPD_408;
    char *sync_ok = net_app_ntp_sync_ok() ? "true" : "false";
    sprintf(content, "{\"sync_ok\":%s}", sync_ok);

    httpd_resp_set_status(req, status);
    httpd_resp_set_type(req, HTTPD_TYPE_JSON);
    httpd_resp_sendstr(req, content);

    return ESP_OK;
}

static esp_err_t net_ntp_index_handler(httpd_req_t *req)
{
    char content[32];
    char *sync_ok = net_app_ntp_sync_ok() ? "true" : "false";
    sprintf(content, "{\"sync_ok\":%s", sync_ok);

    httpd_resp_set_status(req, HTTPD_200);
    httpd_resp_set_type(req, HTTPD_TYPE_JSON);
    httpd_resp_sendstr(req, content);

    return ESP_OK;
}

static esp_err_t net_mqtt_post_handler(httpd_req_t *req)
{
    char content[256];
    net_app_queue_msg_t msg;
    httpd_req_recv(req, content, sizeof(content));

    msg.id = NET_APP_MSG_ID_START_MQTT;
    esp_mqtt_client_config_from_json(&msg.data.mqtt, content);
    net_app_send_msg(&msg);
    net_app_wait_msg_processing(MQTT_TIMEOUT);

    char *status = net_app_mqtt_connected() ? HTTPD_200 : HTTPD_408;
    char *connected = net_app_mqtt_connected() ? "true" : "false";
    sprintf(content, "{\"connected\":%s}", connected);

    httpd_resp_set_status(req, status);
    httpd_resp_set_type(req, HTTPD_TYPE_JSON);
    httpd_resp_sendstr(req, content);

    return ESP_OK;
}

static esp_err_t net_mqtt_index_handler(httpd_req_t *req)
{
    char content[32];
    char *connected = net_app_mqtt_connected() ? "true" : "false";
    sprintf(content, "{\"connected\":%s}", connected);

    httpd_resp_set_status(req, HTTPD_200);
    httpd_resp_set_type(req, HTTPD_TYPE_JSON);
    httpd_resp_sendstr(req, content);

    return ESP_OK;
}

static esp_err_t system_reset_index_handler(httpd_req_t *req)
{
    httpd_resp_set_status(req, HTTPD_200);
    httpd_resp_send(req, NULL, 0);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    esp_restart();

    return ESP_OK;
}

static esp_err_t file_get_handler(httpd_req_t *req)
{
    FILE *fd = NULL;
    char filename[517];
    struct stat file_stat;

    if(strcmp(req->uri, "/") == 0)
    {
        sprintf(filename, "/web%s", HOMEPAGE);
    }
    else
    {
        sprintf(filename, "/web%s", req->uri);
        if (stat(filename, &file_stat) == -1) {
            ESP_LOGE(TAG, "File %s not exists", filename);
            httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "File does not exist");
            return ESP_ERR_NOT_FOUND;
        }
    }

    set_content_type_from_file(req, filename);

    fd = fopen(filename, "r");
    if (!fd) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to read existing file");
        return ESP_FAIL;
    }

    char *chunk = ((file_server_t *)req->user_ctx)->buf;
    size_t chunksize;

    do {
        chunksize = fread(chunk, 1, FILE_SERVER_BUFSIZE, fd);
        if (chunksize > 0) {
            if (httpd_resp_send_chunk(req, chunk, chunksize) != ESP_OK) {
                fclose(fd);
                httpd_resp_sendstr_chunk(req, NULL);
                httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to send file");
               return ESP_FAIL;
           }
        }
    } while (chunksize != 0);

    fclose(fd);

    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

static esp_err_t set_content_type_from_file(httpd_req_t *req, const char *filename)
{
    if (IS_FILE_EXT(filename, ".pdf"))
    {
        return httpd_resp_set_type(req, "application/pdf");
    }
    else if (IS_FILE_EXT(filename, ".html"))
    {
        return httpd_resp_set_type(req, "text/html");
    }
    else if (IS_FILE_EXT(filename, ".jpeg"))
    {
        return httpd_resp_set_type(req, "image/jpeg");
    }
    else if (IS_FILE_EXT(filename, ".ico"))
    {
        return httpd_resp_set_type(req, "image/x-icon");
    }
    else if (IS_FILE_EXT(filename, ".json"))
    {
        return httpd_resp_set_type(req, "application/json");
    }
    else if (IS_FILE_EXT(filename, ".css"))
    {
        return httpd_resp_set_type(req, "text/css");
    }
    else if (IS_FILE_EXT(filename, ".png"))
    {
        return httpd_resp_set_type(req, "image/png");
    }
    else if (IS_FILE_EXT(filename, ".js"))
    {
        return httpd_resp_set_type(req, "application/javascript");
    }
    /* This is a limited set only */
    /* For any other type always set as plain text */
    return httpd_resp_set_type(req, "text/plain");
}