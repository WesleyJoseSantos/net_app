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

void net_app_wifi_sta_info_to_json(char *json_str, net_app_wifi_sta_info_t *data)
{
    cJSON *json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "ip", data->conn.ip);
    cJSON_AddBoolToObject(json, "status", data->conn.status);
    cJSON_AddStringToObject(json, "ssid", (char*)data->ap_record.ssid);
    cJSON_AddNumberToObject(json, "rssi", data->ap_record.rssi);
    cJSON_ToString(json_str, json);
}

void wifi_ap_record_to_json(char *json_str, wifi_ap_record_t *data, size_t size)
{
    int len = sprintf(json_str, "[");
    for (int i = 0; i < size; i++)
    {
        len += sprintf(&json_str[len], 
                       "{\"ssid\":\"%s\",\"rssi\":%d, \"authmode\":%d},", 
                       data[i].ssid, data[i].rssi, data[i].authmode);
    }
    json_str[len - 1] = ']';
}

void net_app_ip_config_from_json(net_app_ip_config_t *data, char *json_str)
{
    cJSON *json = cJSON_Parse(json_str);
    cJSON *dhcp = cJSON_GetObjectItem(json, "dhcp");
    cJSON *ip = cJSON_GetObjectItem(json, "ip");
    cJSON *gateway = cJSON_GetObjectItem(json, "gateway");
    cJSON *mask = cJSON_GetObjectItem(json, "mask");
    cJSON *dns = cJSON_GetObjectItem(json, "dns");
    data->dhcp = dhcp ? dhcp->valueint : 1;
    if(!ip && !gateway && !mask)
    {
        data->dhcp = true;
    }
    else
    {
        if(ip && ip->valuestring) data->ip_info.ip.addr = ipaddr_addr(ip->valuestring);
        if(mask && mask->valuestring) data->ip_info.netmask.addr = ipaddr_addr(mask->valuestring);
        if(gateway && gateway->valuestring) data->ip_info.gw.addr = ipaddr_addr(gateway->valuestring);
        data->dns_info.ip.type = IPADDR_TYPE_V4;
        if(dns && dns->valuestring)
        {
            data->dns_info.ip.u_addr.ip4.addr = ipaddr_addr(dns->valuestring);
        }
        else if(gateway && gateway->valuestring)
        {
            data->dns_info.ip.u_addr.ip4.addr = ipaddr_addr(gateway->valuestring);
        }
    }
    cJSON_Delete(json);
}

void net_app_ip_config_to_json(char *json_str, net_app_ip_config_t *data)
{
   cJSON *json = cJSON_CreateObject();
   cJSON_AddNumberToObject(json, "dhcp", data->dhcp);
    if(!data->dhcp)
    {
        char ip[16], mask[16], gateway[16], dns[16];
        sprintf(ip, IPSTR, IP2STR(&data->ip_info.ip));
        sprintf(mask, IPSTR, IP2STR(&data->ip_info.netmask));
        sprintf(gateway, IPSTR, IP2STR(&data->ip_info.gw));
        sprintf(dns, IPSTR, IP2STR(&data->dns_info.ip.u_addr.ip4));
        if(ip[0] != '\0') cJSON_AddStringToObject(json, "ip", ip);
        if(mask[0] != '\0') cJSON_AddStringToObject(json, "mask", mask);
        if(gateway[0] != '\0') cJSON_AddStringToObject(json, "gateway", gateway);
        if(dns[0] != '\0') cJSON_AddStringToObject(json, "dns", dns);
    }
}

void wifi_sta_config_from_json(wifi_sta_config_t *data, char *json_str)
{
    cJSON *json = cJSON_Parse(json_str);
    cJSON *ssid = cJSON_GetObjectItem(json, "ssid");
    cJSON *password = cJSON_GetObjectItem(json, "password");
    memset(data, 0, sizeof(*data));
    if(ssid && ssid->valuestring[0] != '\0') strncpy((char*)data->ssid, ssid->valuestring, sizeof(data->ssid));
    if(password && password->valuestring[0] != '\0') strncpy((char*)data->password, password->valuestring, sizeof(data->password));
    cJSON_Delete(json);
}

void wifi_sta_config_to_json(char *json_str, wifi_sta_config_t *data)
{
    cJSON *json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "ssid", (char*)&data->ssid);
    cJSON_AddStringToObject(json, "password", (char*)&data->password);
    cJSON_ToString(json_str, json);
}

void net_app_ntp_config_from_json(net_app_ntp_config_t *data, char*json_str)
{
    cJSON *json = cJSON_Parse(json_str);
    cJSON *op_mode = cJSON_GetObjectItem(json, "op_mode");
    cJSON *sync_interval = cJSON_GetObjectItem(json, "sync_interval");
    cJSON *sync_mode = cJSON_GetObjectItem(json, "sync_mode");
    cJSON *server1 = cJSON_GetObjectItem(json, "server1");
    cJSON *server2 = cJSON_GetObjectItem(json, "server2");
    cJSON *server3 = cJSON_GetObjectItem(json, "server3");
    if(op_mode) data->op_mode = op_mode->valueint;
    if(sync_interval) data->sync_interval = op_mode->valueint;
    if(sync_mode) data->sync_mode = sync_mode->valueint;
    if(server1 && server1->valuestring[0] != '\0') strncpy(data->server1, server1->valuestring, sizeof(data->server1));
    if(server2 && server2->valuestring[0] != '\0') strncpy(data->server2, server2->valuestring, sizeof(data->server2));
    if(server3 && server3->valuestring[0] != '\0') strncpy(data->server3, server3->valuestring, sizeof(data->server3));
    cJSON_Delete(json);
}

void net_app_ntp_config_to_json(char *json_str, net_app_ntp_config_t *data)
{
    cJSON *json = cJSON_CreateObject();
    cJSON_AddNumberToObject(json, "op_mode", data->op_mode);
    cJSON_AddNumberToObject(json, "sync_interval", data->sync_interval);
    cJSON_AddNumberToObject(json, "sync_mode", data->sync_mode);
    cJSON_AddStringToObject(json, "server1", data->server1);
    cJSON_AddStringToObject(json, "server2", data->server2);
    cJSON_AddStringToObject(json, "server3", data->server3);
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
    cJSON *ip_cfg = cJSON_GetObjectItem(json, "ip_cfg");
    cJSON *ip_cfg_sta = cJSON_GetArrayItem(ip_cfg, NET_APP_INTERFACE_WIFI_STA);
    cJSON *ip_cfg_ap = cJSON_GetArrayItem(ip_cfg, NET_APP_INTERFACE_WIFI_AP);
    cJSON *ip_cfg_eth = cJSON_GetArrayItem(ip_cfg, NET_APP_INTERFACE_ETH);
    cJSON *wifi = cJSON_GetObjectItem(json, "wifi");
    cJSON *ntp = cJSON_GetObjectItem(json, "ntp");
    cJSON *mqtt = cJSON_GetObjectItem(json, "mqtt");
    char ip_cfg_sta_json = cJSON_Print(ip_cfg_sta);
    char ip_cfg_ap_json = cJSON_Print(ip_cfg_ap);
    char ip_cfg_eth_json = cJSON_Print(ip_cfg_eth);
    char *wifi_json = cJSON_Print(wifi);
    char *ntp_json = cJSON_Print(ntp);
    char *mqtt_json = cJSON_Print(mqtt);
    net_app_ip_config_from_json(&data->ip_cfg[NET_APP_INTERFACE_WIFI_STA], ip_cfg_sta_json);
    net_app_ip_config_from_json(&data->ip_cfg[NET_APP_INTERFACE_WIFI_AP], ip_cfg_ap_json);
    net_app_ip_config_from_json(&data->ip_cfg[NET_APP_INTERFACE_ETH], ip_cfg_eth);
    wifi_sta_config_from_json(&data->wifi_sta, wifi_json);
    net_app_ntp_config_from_json(&data->ntp, ntp_json);
    esp_mqtt_client_config_from_json(&data->mqtt, mqtt_json);
    free(wifi_json);
    free(ntp_json);
    free(mqtt_json);
    cJSON_Delete(json);
}

void net_app_settings_to_json(char *json_str, net_app_settings_t *data)
{
    char wifi_json[256];
    char ntp_json[256];
    char mqtt_json[256];
    wifi_sta_config_to_json(wifi_json, &data->wifi_sta);
    net_app_ntp_config_to_json(ntp_json, &data->ntp);
    esp_mqtt_client_config_to_json(mqtt_json, &data->mqtt);
    cJSON *json = cJSON_CreateObject();
    cJSON *wifi = cJSON_Parse(wifi_json);
    cJSON *ntp = cJSON_Parse(ntp_json);
    cJSON *mqtt = cJSON_Parse(mqtt_json);
    cJSON_AddItemToObject(json, "wifi", wifi);
    cJSON_AddItemToObject(json, "ntp", ntp);
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
