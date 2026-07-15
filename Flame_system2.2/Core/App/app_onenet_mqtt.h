#ifndef APP_ONENET_MQTT_H_
#define APP_ONENET_MQTT_H_

#include "stdint.h"
#include "string.h"
#include "../esp8266/esp8266.h"

#define ONENET_DATA_NUM 4

typedef enum {
    DATA_FLOAT = 0,
    DATA_UINT32,
    DATA_INT32,
    DATA_BOOL,
    DATA_STRING
} data_type_t;

typedef union {
    float f32_val;
    uint32_t u32_val;
    int32_t i32_val;
    uint8_t bool_val;
    char string_val[32];
} data_value_t;

typedef struct {
    char identifier[32];
    data_type_t type;
    data_value_t value;
} onenet_pub_data_t;

typedef struct {
    char topic[128];
    char json_data[512];
    uint16_t json_len;
} onenet_mqtt_topic_data_t;

typedef struct {
    char msg_id[32];
    onenet_pub_data_t data;
} onenet_mqtt_propertySet_t;

extern mqtt_info_t mqtt_info;
extern onenet_pub_data_t onenet_data[ONENET_DATA_NUM];

uint8_t onenet_mqtt_publish_post_data(onenet_pub_data_t *data, uint8_t num);
uint8_t onenet_mqtt_get_topic_data(char *topic, char *data, onenet_mqtt_topic_data_t *topic_data);
void onenet_mqtt_topic_log(onenet_mqtt_topic_data_t *topic_data);
uint8_t onenet_mqtt_parse_propertySet_topic(char *json_data, onenet_mqtt_propertySet_t *propertySet);
uint8_t onenet_mqtt_response_topic(char *msg_id, int code, char *msg);

#endif /* APP_ONENET_MQTT_H_ */