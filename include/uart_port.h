#ifndef UART_PORT_H
#define UART_PORT_H

#include <stdint.h>
#include <stdbool.h>
#include "driver/uart.h"

typedef struct {
    uart_port_t uart_num;   // UART_NUM_1 or 2
    int tx_gpio;
    int rx_gpio;
    int baud;
    int rx_timeout_ms;      // for state machine watchdog
} UartPortCfg;

typedef struct {
    UartPortCfg cfg;
    QueueHandle_t evt_q;    // from esp-idf driver
} UartPort;

bool up_init(UartPort *up, const UartPortCfg *cfg);
int  up_read_bytes(UartPort *up, uint8_t *buf, int maxlen, int timeout_ms);
int  up_write_bytes(UartPort *up, const uint8_t *buf, int len);

#endif
