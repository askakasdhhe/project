/*
 * esp8266.c
 *
 *  Created on: 2024年6月14日
 *      Author: Benny
 */
#include "stdio.h"
#include "string.h"
#include "esp8266.h"
#include "../Middlewares/EmbATlink/at_driver.h"

#define AT_LUN_ESP8266 0

#define ESP_LOG(...) printf(__VA_ARGS__)

uint8_t esp8266_init(uint8_t first)
{
    char *p_recv = NULL;
    uint8_t index = 0;

    index++;
    if (at_cmd_format_send_and_recv(AT_LUN_ESP8266, NULL, ESP_AT, "") != 0)
        goto error;

    index++;
    if (first == 1)
    {
        if (at_cmd_format_send_and_recv(AT_LUN_ESP8266, NULL, ESP_RESTORE, "") != 0)
            goto error;
    }
    else
    {
        if (at_cmd_format_send_and_recv(AT_LUN_ESP8266, NULL, ESP_RST, "") != 0)
            goto error;
    }

    index++;
    if (at_cmd_format_send_and_recv(AT_LUN_ESP8266, NULL, ESP_AT, "") != 0)
        goto error;

    index++;
    if (at_cmd_format_send_and_recv(AT_LUN_ESP8266, NULL, ESP_ATE, "0") != 0)
        goto error;

    return 0;

error:
    ESP_LOG("[ESP8266][ERROR] init err:%d\r\n", index);
    return index;
}

uint8_t esp8266_wifi_connect(char *ssid, char *pwd)
{
    char *p_recv = NULL;
    uint8_t index = 0;

    index++;
    if (at_cmd_format_send_and_recv(AT_LUN_ESP8266, NULL, ESP_WIFI_SET_MODE, "1") != 0)
        goto error;

    index++;
    if (at_cmd_format_send_and_recv(AT_LUN_ESP8266, NULL, ESP_WIFI_CONNECT, "\"%s\",\"%s\"", ssid, pwd) != 0)
        goto error;

    return 0;

error:
    ESP_LOG("[ESP8266][ERROR] wifi_connect err:%d\r\n", index);
    return index;
}

uint8_t esp8266_mqtt_connect(char *client_id, char *username, char *password, char *host, uint16_t port)
{
    char *p_recv = NULL;
    uint8_t index = 0;

    index++;
    if (at_cmd_format_send_and_recv(AT_LUN_ESP8266, NULL, ESP_MQTT_SET_INFO,
                                    "0,1,\"%s\",\"%s\",\"%s\",0,0,\"\"",
                                    client_id, username, password) != 0)
        goto error;

    index++;
    if (at_cmd_format_send_and_recv(AT_LUN_ESP8266, NULL, ESP_MQTT_CONNECT, "0,\"%s\",%d,1", host, port) != 0)
        goto error;

    return 0;

error:
    ESP_LOG("[ESP8266][ERROR] mqtt_connect err:%d\r\n", index);
    return index;
}

uint8_t esp8266_mqtt_subscribe(char *topic, uint8_t qos)
{
    char *p_recv = NULL;
    uint8_t index = 0;

    index++;
    if (at_cmd_format_send_and_recv(AT_LUN_ESP8266, NULL, ESP_MQTT_SUBSCRIBE, "0,\"%s\",%d", topic, qos) != 0)
        goto error;

    return 0;

error:
    ESP_LOG("[ESP8266][ERROR] mqtt_subscribe err:%d\r\n", index);
    return index;
}

uint8_t esp8266_mqtt_publish(char *topic, char *data, uint16_t len, uint8_t qos, uint8_t retain)
{
    char *p_recv = NULL;
    uint8_t index = 0;

    index++;
    if (at_cmd_format_send_and_recv(AT_LUN_ESP8266, NULL, ESP_MQTT_PUBLISH_RAW, "0,\"%s\",%d,%d,%d", topic, len, qos, retain) != 0)
        goto error;

    if (at_cmd_format_send_and_recv(AT_LUN_ESP8266, NULL, ESP_MQTT_PUBLISH_DATA, "%s", data) != 0)
        goto error;

    return 0;

error:
    ESP_LOG("[ESP8266][ERROR] mqtt_publish err:%d\r\n", index);
    return index;
}