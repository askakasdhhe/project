#ifndef AT_DRIVER_H
#define AT_DRIVER_H

#include "stdint.h"

#define AT_LUN_MAX          1
#define AT_RECV_BUFFER_SIZE 1024
#define AT_SEND_BUFFER_SIZE 256

#define AT_LOG(...)         printf(__VA_ARGS__)
#define AT_LOG_I(...)       printf(__VA_ARGS__)
#define AT_LOG_W(...)       printf(__VA_ARGS__)
#define AT_LOG_E(...)       printf(__VA_ARGS__)

typedef enum
{
    AT_CMD_DEFAULT,
    ESP_AT,
    ESP_RST,
    ESP_RESTORE,
    ESP_ATE,
    ESP_WIFI_SET_MODE,
    ESP_WIFI_CONNECT,
    ESP_MQTT_SET_INFO,
    ESP_MQTT_CONNECT,
    ESP_MQTT_SUBSCRIBE,
    ESP_MQTT_PUBLISH_RAW,
    ESP_MQTT_PUBLISH_DATA,
    AT_CMD_LAST,
} at_cmd_id_e;

typedef enum
{
    AT_MONITOR_DEFAULT,
    ESP_MQTT_RECV_DATA,
    AT_MONITOR_LAST,
} at_monitor_id_e;

typedef struct
{
    char *cmd_str;
    char *expected_rsp;
    uint8_t send_count;
    uint16_t check_interval_ms;
    uint16_t recv_timeout_ms;
} at_cmd_config_t;

void at_init(void);
void at_clear_recv_buffer(uint8_t lun);
void at_uart_recv_handler(uint8_t lun, const uint8_t *data, uint16_t len);
char *at_get_recv_buffer(uint8_t lun);
uint8_t at_get_monitor_match_index(uint8_t lun);
uint8_t at_cmd_format_send_and_recv(uint8_t lun, char **recv_buffer, at_cmd_id_e cmd_id, char *format, ...);

#endif