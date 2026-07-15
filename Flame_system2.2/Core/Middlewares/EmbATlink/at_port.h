#ifndef AT_PORT_H
#define AT_PORT_H

#include "stdint.h"

void at_port_delay_ms(uint32_t xms);
uint32_t at_port_get_tick_ms(void);
void at_port_init(uint8_t lun);
void at_port_uart_transmit(uint8_t lun, const char *buf, uint16_t len);
void at_port_enter_critical(uint8_t lun);
void at_port_exit_critical(uint8_t lun);

#endif