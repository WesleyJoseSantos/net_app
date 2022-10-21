/**
 * @file json.c
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2022-10-01
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include "dtos.h"
#include "cJSON.h"

static void cJSON_ToString(char *str, cJSON *obj);

void wifi_sta_config_from_json(wifi_sta_config_t *data, char *json_str)
{
    cJSON *json = cJSON_Parse(json_str);
    cJSON *ssid = cJSON_GetObjectItem(json, "ssid");
    cJSON *password = cJSON_GetObjectItem(json, "password");
    memset(data, 0, sizeof(*data));
    if(ssid && ssid->valuestring[0] != '\0') strncpy((char*)data->ssid, ssid->valuestring, 32);
    if(password && password->valuestring[0] != '\0') strncpy((char*)data->password, password->valuestring, 64);
    cJSON_Delete(json);
}

void wifi_sta_config_to_json(char *json_str, wifi_sta_config_t *data)
{
    cJSON *json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "ssid", (char*)&data->ssid);
    cJSON_AddStringToObject(json, "password", (char*)&data->password);
    cJSON_ToString(json_str, json);
}

void net_app_wifi_sta_info_to_json(char *json_str, net_app_wifi_sta_info_t *data)
{
    cJSON *json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "ssid", data->conn.ssid);
    cJSON_AddStringToObject(json, "ip", data->conn.ip);
    cJSON_AddBoolToObject(json, "status", data->conn.status);
    cJSON_AddNumberToObject(json, "rssi", data->conn.rssi);
    cJSON_ToString(json_str, json);
}

void esp_mqtt_client_config_from_json(esp_mqtt_client_config_t *data, char *json_str)
{
    cJSON *json = cJSON_Parse(json_str);
    cJSON *host = cJSON_GetObjectItem(json, "host");
    cJSON *port = cJSON_GetObjectItem(json, "port");
    cJSON *username = cJSON_GetObjectItem(json, "username");
    cJSON *password = cJSON_GetObjectItem(json, "password");
    cJSON *transport = cJSON_GetObjectItem(json, "transport");
    memset(data, 0, sizeof(*data));
    if(host && host->valuestring[0] != '\0') data->host = malloc(strlen(host->valuestring));
    if(username && username->valuestring[0] != '\0') data->username = malloc(strlen(username->valuestring));
    if(password && password->valuestring[0] != '\0') data->password = malloc(strlen(password->valuestring));
    if(host && host->valuestring[0] != '\0') strcpy((char*)data->host, host->valuestring);
    if(port) data->port = port->valueint;
    if(username && username->valuestring[0] != '\0') strcpy((char*)data->username, username->valuestring);
    if(password && password->valuestring[0] != '\0') strcpy((char*)data->password, password->valuestring);
    if(transport) data->transport = transport->valueint;
    cJSON_Delete(json);
}

void esp_mqtt_client_config_to_json(char *json_str, esp_mqtt_client_config_t *data)
{
    cJSON *json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "host", data->host);
    cJSON_AddNumberToObject(json, "port", data->port);
    cJSON_AddStringToObject(json, "username", data->username);
    cJSON_AddStringToObject(json, "password", data->password);
    cJSON_AddNumberToObject(json, "transport", data->transport);
    cJSON_ToString(json_str, json);
}

void net_app_settings_from_json(net_app_settings_t *data, char *json_str)
{
    cJSON *json = cJSON_Parse(json_str);
    cJSON *wifi = cJSON_GetObjectItem(json, "wifi");
    cJSON *mqtt = cJSON_GetObjectItem(json, "mqtt");
    char *wifi_json = cJSON_Print(wifi);
    char *mqtt_json = cJSON_Print(mqtt);
    wifi_sta_config_from_json(&data->wifi_sta, wifi_json);
    esp_mqtt_client_config_from_json(&data->mqtt, mqtt_json);
    free(wifi_json);
    free(mqtt_json);
    cJSON_Delete(json);
}

void net_app_settings_to_json(char *json_str, net_app_settings_t *data)
{
    char wifi_json[256];
    char mqtt_json[256];
    wifi_sta_config_to_json(wifi_json, &data->wifi_sta);
    esp_mqtt_client_config_to_json(mqtt_json, &data->mqtt);
    cJSON *json = cJSON_CreateObject();
    cJSON *wifi = cJSON_Parse(wifi_json);
    cJSON *mqtt = cJSON_Parse(mqtt_json);
    cJSON_AddItemToObject(json, "wifi", wifi);
    cJSON_AddItemToObject(json, "mqtt", mqtt);
    cJSON_ToString(json_str, json);
}

static void cJSON_ToString(char *str, cJSON *obj)
{
    char *buf = cJSON_Print(obj);
    sprintf(str, buf);
    cJSON_Delete(obj);
    free(buf);
}