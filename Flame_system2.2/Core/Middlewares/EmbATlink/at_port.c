#include "at_port.h"
#include "at_driver.h"

#include "main.h"
#include "usart.h"

#define AT_LUN_ESP8266 0

const at_cmd_config_t at_cmd_table[AT_CMD_LAST] = {
    [AT_CMD_DEFAULT]        =   {"AT_CMD_DEFAULT",      NULL,   0,  0,  0},
    [ESP_AT]                =   {"AT",                  "OK",   50, 20, 200},
    [ESP_RST]               =   {"AT+RST",              "OK",   3,  20, 8000},
    [ESP_RESTORE]           =   {"AT+RESTORE",          "OK",   3,  20, 8000},
    [ESP_ATE]               =   {"ATE",                 "OK",   3,  20, 200},
    [ESP_WIFI_SET_MODE]     =   {"AT+CWMODE=",          "OK",   3,  20, 200},
    [ESP_WIFI_CONNECT]      =   {"AT+CWJAP=",           "OK",   3,  20, 8000},
    [ESP_MQTT_SET_INFO]     =   {"AT+MQTTUSERCFG=",     "OK",   3,  20, 200},
    [ESP_MQTT_CONNECT]      =   {"AT+MQTTCONN=",        "OK",   3,  20, 2000},
    [ESP_MQTT_SUBSCRIBE]    =   {"AT+MQTTSUB=",         "OK",   3,  20, 200},
    [ESP_MQTT_PUBLISH_RAW]  =   {"AT+MQTTPUBRAW=",      NULL,   1,  20, 100},
    [ESP_MQTT_PUBLISH_DATA] =   {"",                    "OK",   1,  20, 200},
};

const char *at_monitor_key_table[AT_MONITOR_LAST] = {
    [AT_MONITOR_DEFAULT]    =   "AT_MONITOR_DEFAULT",
    [ESP_MQTT_RECV_DATA]    =   "+MQTTSUBRECV:",
};

uint8_t at_rx_data[AT_LUN_MAX][1];

void at_port_delay_ms(uint32_t xms)
{
    HAL_Delay(xms);
}

uint32_t at_port_get_tick_ms(void)
{
    return HAL_GetTick();
}

void at_port_init(uint8_t lun)
{
    switch (lun)
    {
    case AT_LUN_ESP8266:
        HAL_UART_Receive_IT(&huart2, (uint8_t *)at_rx_data[lun], 1);
        break;
    default:
        break;
    }
}

void at_port_uart_transmit(uint8_t lun, const char *buf, uint16_t len)
{
    switch (lun)
    {
    case AT_LUN_ESP8266:
        HAL_UART_Transmit(&huart2, (uint8_t *)buf, len, 1000);
        break;
    default:
        break;
    }
}

void at_port_enter_critical(uint8_t lun)
{
    (void)lun;
}

void at_port_exit_critical(uint8_t lun)
{
    (void)lun;
}