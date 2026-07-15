#include "at_driver.h"

#include <stdarg.h>
#include <string.h>
#include <stddef.h>

char at_recv_buffer[AT_LUN_MAX][AT_RECV_BUFFER_SIZE];
static char at_send_buffer[AT_LUN_MAX][AT_SEND_BUFFER_SIZE];

uint16_t volatile at_rx_status[AT_LUN_MAX];
static uint8_t at_monitor_match_index[AT_LUN_MAX];

extern const at_cmd_config_t at_cmd_table[AT_CMD_LAST];
extern const char *at_monitor_key_table[AT_MONITOR_LAST];

void at_init(void)
{
    for (uint8_t i = 0; i < AT_LUN_MAX; i++)
    {
        memset(at_recv_buffer[i], 0, AT_RECV_BUFFER_SIZE);
        memset(at_send_buffer[i], 0, AT_SEND_BUFFER_SIZE);
        at_rx_status[i] = 0U;
        at_monitor_match_index[i] = 0xFFU;
    }

    for (uint8_t i = 0; i < AT_LUN_MAX; i++)
    {
        at_port_init(i);
    }
}

void at_clear_recv_buffer(uint8_t lun)
{
    memset(at_recv_buffer[lun], 0, AT_RECV_BUFFER_SIZE);
    at_rx_status[lun] = 0;
}

static uint8_t at_monitor_data_match(uint8_t lun)
{
    for (uint8_t i = 0; i < AT_MONITOR_LAST; i++)
    {
        if (strstr((const char *)at_recv_buffer[lun], at_monitor_key_table[i]) != NULL)
            return i;
    }
    return 0xFF;
}

void at_uart_recv_handler(uint8_t lun, const uint8_t *data, uint16_t len)
{
    uint16_t data_len = at_rx_status[lun] & 0x7FFF;

    if ((data_len + len) >= AT_RECV_BUFFER_SIZE)
    {
        at_rx_status[lun] = 0;
        AT_LOG_E("[AT:%d][ERR] RECV BUFFER OVERFLOW!\r\n", lun);
        return;
    }

    memcpy(at_recv_buffer[lun] + data_len, data, len);
    at_rx_status[lun] += len;
    data_len += len;

    if ((at_recv_buffer[lun][0] == '>') ||
        (data_len > 2U && (at_recv_buffer[lun][data_len - 1] == '\n' && at_recv_buffer[lun][data_len - 2] == '\r')))
    {
        at_monitor_match_index[lun] = at_monitor_data_match(lun);
        at_rx_status[lun] |= 0x8000U;
    }
}

char *at_get_recv_buffer(uint8_t lun)
{
    return &at_recv_buffer[lun][0];
}

uint8_t at_get_monitor_match_index(uint8_t lun)
{
    uint8_t temp = at_monitor_match_index[lun];
    at_monitor_match_index[lun] = 0xFF;
    return temp;
}

static uint8_t at_cmd_send_and_wait(uint8_t lun, char *cmd, char **out_recv, const at_cmd_config_t *cmd_config)
{
    uint8_t send_cnt = 0;
    uint8_t res = 1;
    at_port_enter_critical(lun);
    for (uint8_t i = 0; i < cmd_config->send_count; i++)
    {
        at_clear_recv_buffer(lun);

        at_port_uart_transmit(lun, cmd, strlen(cmd));
        at_port_uart_transmit(lun, "\r\n", 2);
        send_cnt++;

        if (cmd_config->expected_rsp == NULL)
        {
            at_port_delay_ms(cmd_config->recv_timeout_ms);
            AT_LOG_I("[AT][SUCC] CMD:%s\r\n", cmd);
            if (out_recv != NULL)
                *out_recv = &at_recv_buffer[lun][0];
            res = 0;
            break;
        }

        uint32_t recv_tick = at_port_get_tick_ms();

        while (at_port_get_tick_ms() - recv_tick <= cmd_config->recv_timeout_ms)
        {
            at_port_delay_ms(cmd_config->check_interval_ms);

            if ((at_rx_status[lun] & 0x8000) == 0)
                continue;

            at_rx_status[lun] &= 0x7FFF;

            if (strstr((const char *)at_recv_buffer[lun], cmd_config->expected_rsp) != NULL)
            {
                if (out_recv != NULL)
                    *out_recv = &at_recv_buffer[lun][0];

                res = 0;
                break;
            }

            res = 2;
        }

        if (res == 0)
            AT_LOG_I("[AT][SUCC] CMD:%s\r\n", cmd);
        if (res == 0)
            break;
        else if (res == 1)
            AT_LOG_W("[AT][WARN][%hhu] CMD:%s, TIME OUT\r\n", send_cnt, cmd);
        else if (res == 2)
            AT_LOG_E("[AT][WARN][%hhu] CMD:%s, RECV: %s\r\n", send_cnt, cmd, at_recv_buffer[lun]);
    }

    if (res != 0)
        AT_LOG_E("[AT:%d][ERR] CMD:%s\r\n", lun, cmd);

    at_port_exit_critical(lun);

    return res;
}

uint8_t at_cmd_format_send_and_recv(uint8_t lun, char **recv_buffer, at_cmd_id_e cmd_id, char *format, ...)
{
    uint8_t res = 0;

    memset(at_send_buffer[lun], 0, sizeof(at_send_buffer[lun]));
    va_list arg;
    va_start(arg, format);
    snprintf(at_send_buffer[lun], sizeof(at_send_buffer[lun]), "%s", at_cmd_table[cmd_id].cmd_str);
    vsnprintf(at_send_buffer[lun] + strlen(at_send_buffer[lun]), sizeof(at_send_buffer[lun]) - strlen(at_send_buffer[lun]), format, arg);
    va_end(arg);

    if (strlen(at_send_buffer[lun]) == (sizeof(at_send_buffer[lun]) - 1))
        return 3;

    res = at_cmd_send_and_wait(lun, at_send_buffer[lun],
                               recv_buffer,
                               &at_cmd_table[cmd_id]);

    return res;
}