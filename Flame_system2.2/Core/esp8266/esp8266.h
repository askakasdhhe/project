#ifndef INC_ESP8266_H_
#define INC_ESP8266_H_

#include "stdint.h"

#define AT_LUN_ESP8266 0

typedef struct {
    char ssid[32];
    char password[64];
} wifi_info_t;

typedef struct {
    char host[64];
    uint16_t port;
    char client_id[64];
    char username[128];
    char password[256];
    char publish_topic_post[128];
    char subscribe_topic_post[128];
    char publish_topic_set[128];
    char subscribe_topic_set[128];
} mqtt_info_t;

uint8_t esp8266_init(uint8_t first);
uint8_t esp8266_wifi_connect(char *ssid, char *pwd);
uint8_t esp8266_mqtt_connect(char *client_id, char *username, char *password, char *host, uint16_t port);
uint8_t esp8266_mqtt_subscribe(char *topic, uint8_t qos);
uint8_t esp8266_mqtt_publish(char *topic, char *data, uint16_t len, uint8_t qos, uint8_t retain);

#endif /* INC_ESP8266_H_ */