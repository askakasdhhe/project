#include "app_onenet_mqtt.h"
#include "../esp8266/esp8266.h"
#include "../cJSON/cJSON.h"
#include "stdio.h"
#include "string.h"

#define ONENET_PROTOCOL_VERSION "1.0"

extern volatile uint8_t mqttconnetState;
#define CONNECTED 1
#define DISCONNECTED 0

onenet_pub_data_t onenet_data[ONENET_DATA_NUM] = {
    {"fire", DATA_FLOAT, {.f32_val = 0}},
    {"beep", DATA_INT32, {.i32_val = 0}},
    {"aiuser", DATA_BOOL, {.bool_val = 0}},
    {"ADIO", DATA_BOOL, {.bool_val = 0}}
};

uint8_t onenet_mqtt_publish_post_data(onenet_pub_data_t *data, uint8_t num)
{
    if (mqttconnetState != CONNECTED)
    {
        printf("[ONENET][ERROR] MQTT not connected, skip publish\r\n");
        return 1;
    }

    cJSON *root = cJSON_CreateObject();
    cJSON *params = cJSON_CreateObject();
    char *json_str = NULL;
    uint16_t json_len = 0;
    uint8_t ret = 0;

    if (root == NULL || params == NULL)
    {
        printf("[ONENET][ERROR] cJSON create object failed\r\n");
        if (root) cJSON_Delete(root);
        if (params) cJSON_Delete(params);
        return 1;
    }

    static uint32_t msg_id = 0;
    char id_str[16];
    snprintf(id_str, sizeof(id_str), "%lu", (unsigned long)(++msg_id));
    cJSON_AddStringToObject(root, "id", id_str);
    cJSON_AddStringToObject(root, "version", "1.0");

    for (uint8_t i = 0; i < num; i++)
    {
        cJSON *item = cJSON_CreateObject();
        if (item == NULL) continue;
        switch (data[i].type)
        {
            case DATA_FLOAT:
                cJSON_AddNumberToObject(item, "value", data[i].value.f32_val);
                break;
            case DATA_UINT32:
                cJSON_AddNumberToObject(item, "value", data[i].value.u32_val);
                break;
            case DATA_INT32:
                cJSON_AddNumberToObject(item, "value", data[i].value.i32_val);
                break;
            case DATA_BOOL:
                cJSON_AddBoolToObject(item, "value", data[i].value.bool_val);
                break;
            case DATA_STRING:
                cJSON_AddStringToObject(item, "value", data[i].value.string_val);
                break;
            default:
                cJSON_Delete(item);
                printf("[ONENET][WARN] unknown data type: %d\r\n", data[i].type);
                continue;
        }
        cJSON_AddItemToObject(params, data[i].identifier, item);
    }

    cJSON_AddItemToObject(root, "params", params);

    json_str = cJSON_PrintUnformatted(root);
    if (json_str == NULL)
    {
        printf("[ONENET][ERROR] cJSON print failed\r\n");
        cJSON_Delete(root);
        return 1;
    }

    json_len = strlen(json_str);
    printf("[ONENET][INFO] publish data: %s (len=%d)\r\n", json_str, json_len);

    ret = esp8266_mqtt_publish((char *)mqtt_info.publish_topic_post, json_str, json_len, 0, 0);

    if (ret != 0) {
        printf("[ONENET][ERROR] mqtt publish failed, ret=%d\r\n", ret);
    } else {
        printf("[ONENET][INFO] publish success\r\n");
    }

    cJSON_Delete(root);
    free(json_str);

    return ret;
}

uint8_t onenet_mqtt_get_topic_data(char *topic, char *data, onenet_mqtt_topic_data_t *topic_data)
{
    char *p_topic_start = NULL;
    char *p_topic_end = NULL;
    char *p_json_start = NULL;
    char *p_json_end = NULL;
    char *p_len_start = NULL;
    char *p_len_end = NULL;
    uint16_t json_len = 0;

    if (topic == NULL || data == NULL || topic_data == NULL)
        return 1;

    p_topic_start = strstr(data, topic);
    if (p_topic_start == NULL)
        return 1;

    p_len_start = strchr(p_topic_start, ',');
    if (p_len_start == NULL)
        return 1;

    p_len_end = strchr(p_len_start + 1, ',');
    if (p_len_end == NULL)
        return 1;

    json_len = atoi(p_len_start + 1);

    p_json_start = strchr(p_len_end + 1, '{');
    if (p_json_start == NULL)
        return 1;

    p_json_end = strrchr(data, '}');
    if (p_json_end == NULL)
        return 1;

    strncpy(topic_data->topic, topic, sizeof(topic_data->topic) - 1);
    topic_data->topic[sizeof(topic_data->topic) - 1] = '\0';

    if ((p_json_end - p_json_start + 1) <= sizeof(topic_data->json_data))
    {
        strncpy(topic_data->json_data, p_json_start, p_json_end - p_json_start + 1);
        topic_data->json_data[p_json_end - p_json_start + 1] = '\0';
    }
    else
    {
        strncpy(topic_data->json_data, p_json_start, sizeof(topic_data->json_data) - 1);
        topic_data->json_data[sizeof(topic_data->json_data) - 1] = '\0';
    }

    topic_data->json_len = json_len;

    return 0;
}

void onenet_mqtt_topic_log(onenet_mqtt_topic_data_t *topic_data)
{
    if (topic_data == NULL)
        return;

    printf("[ONENET][TOPIC] topic: %s\r\n", topic_data->topic);
    printf("[ONENET][DATA] json_len: %d\r\n", topic_data->json_len);
    printf("[ONENET][DATA] json_data: %s\r\n", topic_data->json_data);
}

uint8_t onenet_mqtt_parse_propertySet_topic(char *json_data, onenet_mqtt_propertySet_t *propertySet)
{
    cJSON *root = NULL;
    cJSON *id_item = NULL;
    cJSON *version_item = NULL;
    cJSON *params_item = NULL;
    cJSON *param_item = NULL;
    cJSON *value_item = NULL;
    uint8_t ret = 1;

    if (json_data == NULL || propertySet == NULL)
        return 1;

    root = cJSON_Parse(json_data);
    if (root == NULL)
    {
        printf("[ONENET][ERROR] cJSON parse failed\r\n");
        return 1;
    }

    id_item = cJSON_GetObjectItem(root, "id");
    version_item = cJSON_GetObjectItem(root, "version");
    params_item = cJSON_GetObjectItem(root, "params");

    if (id_item == NULL || version_item == NULL || params_item == NULL)
    {
        printf("[ONENET][ERROR] missing required fields\r\n");
        goto exit;
    }

    strncpy(propertySet->msg_id, id_item->valuestring, sizeof(propertySet->msg_id) - 1);
    propertySet->msg_id[sizeof(propertySet->msg_id) - 1] = '\0';

    param_item = cJSON_GetArrayItem(params_item, 0);
    if (param_item == NULL)
    {
        param_item = params_item->child;
        if (param_item == NULL)
        {
            printf("[ONENET][ERROR] params is empty\r\n");
            goto exit;
        }
    }

    strncpy(propertySet->data.identifier, param_item->string, sizeof(propertySet->data.identifier) - 1);
    propertySet->data.identifier[sizeof(propertySet->data.identifier) - 1] = '\0';

    value_item = cJSON_GetObjectItem(params_item, propertySet->data.identifier);
    if (value_item == NULL)
    {
        printf("[ONENET][ERROR] missing value for %s\r\n", propertySet->data.identifier);
        goto exit;
    }

    if (cJSON_IsNumber(value_item))
    {
        if (value_item->valueint == 0 || value_item->valueint == 1)
        {
            propertySet->data.type = DATA_BOOL;
            propertySet->data.value.bool_val = (uint8_t)value_item->valueint;
        }
        else if (value_item->valueint >= 0)
        {
            propertySet->data.type = DATA_UINT32;
            propertySet->data.value.u32_val = (uint32_t)value_item->valueint;
        }
        else
        {
            propertySet->data.type = DATA_INT32;
            propertySet->data.value.i32_val = (int32_t)value_item->valueint;
        }
    }
    else if (cJSON_IsString(value_item))
    {
        propertySet->data.type = DATA_STRING;
        strncpy(propertySet->data.value.string_val, value_item->valuestring, sizeof(propertySet->data.value.string_val) - 1);
        propertySet->data.value.string_val[sizeof(propertySet->data.value.string_val) - 1] = '\0';
    }
    else if (cJSON_IsBool(value_item))
    {
        propertySet->data.type = DATA_BOOL;
        propertySet->data.value.bool_val = (uint8_t)cJSON_IsTrue(value_item);
    }
    else
    {
        printf("[ONENET][WARN] unknown value type\r\n");
        goto exit;
    }

    ret = 0;

exit:
    if (root)
        cJSON_Delete(root);

    return ret;
}

uint8_t onenet_mqtt_response_topic(char *msg_id, int code, char *msg)
{
    cJSON *root = cJSON_CreateObject();
    char *json_str = NULL;
    uint16_t json_len = 0;
    uint8_t ret = 0;

    if (root == NULL)
    {
        printf("[ONENET][ERROR] cJSON create object failed\r\n");
        return 1;
    }

    cJSON_AddStringToObject(root, "id", msg_id);
    cJSON_AddNumberToObject(root, "code", code);
    cJSON_AddStringToObject(root, "msg", msg);

    json_str = cJSON_PrintUnformatted(root);
    if (json_str == NULL)
    {
        printf("[ONENET][ERROR] cJSON print failed\r\n");
        cJSON_Delete(root);
        return 1;
    }

    json_len = strlen(json_str);
    printf("[ONENET][INFO] response data: %s (len=%d)\r\n", json_str, json_len);

    ret = esp8266_mqtt_publish((char *)mqtt_info.publish_topic_set, json_str, json_len, 0, 0);

    cJSON_Delete(root);
    free(json_str);

    return ret;
}