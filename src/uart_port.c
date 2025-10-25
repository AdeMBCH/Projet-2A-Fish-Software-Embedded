#include "uart_port.h"
#include "driver/uart.h"
#include "hal/uart_types.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "esp_err.h"

bool up_init(UartPort *up, const UartPortCfg *cfg){
    up->cfg=*cfg;
    uart_config_t uc = {
        .baud_rate = cfg->baud,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB
    };
    ESP_ERROR_CHECK(uart_driver_install(cfg->uart_num, 2048, 1024, 10, &up->evt_q, 0));
    ESP_ERROR_CHECK(uart_param_config(cfg->uart_num, &uc));
    ESP_ERROR_CHECK(uart_set_pin(cfg->uart_num, cfg->tx_gpio, cfg->rx_gpio, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    return true;
}

int up_read_bytes(UartPort *up, uint8_t *buf, int maxlen, int timeout_ms){
    return uart_read_bytes(up->cfg.uart_num, buf, maxlen, timeout_ms/portTICK_PERIOD_MS);
}
int up_write_bytes(UartPort *up, const uint8_t *buf, int len){
    return uart_write_bytes(up->cfg.uart_num, (const char*)buf, len);
}
