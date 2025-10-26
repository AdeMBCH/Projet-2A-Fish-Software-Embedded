#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "uart_port.h"
#include "ring_buffer.h"
#include "frame_codec.h"
#include "uart_link.h"
#include "msg_router.h"
#include "uart_commands.h"

#include "servo_ctrl.h"

#define RB_RX_SZ   2048
#define PAYLOAD_SZ 512

static UartPort port;
static UartLink link;
static RingBuffer rb_rx;
static uint8_t    rb_rx_mem[RB_RX_SZ];
static FrameCodec codec;
static uint8_t    payload_buf[PAYLOAD_SZ];

static ServoCtrl stail;

static void on_ping(const DecodedFrameMsg *m){
    (void)m;
    const char pong[] = "pong";
    ul_send(&link, UART_CMD_PONG, (const uint8_t*)pong, sizeof(pong)-1);
}

void app_main(void){
    sc_init(&stail, 18);
    st_set_us_limits(&stail.model, 800, 1500, 2200);
    sc_set_profile(&stail, 10.0f, 30.0f, 0.0f); // 1 Hz, ±30°
    sc_start(&stail);

    xTaskCreate(servo_task, "servo_task", 3072, &stail, 8, NULL);


    // // 115200 8N1 on configurable pins
    // UartPortCfg cfg = { .uart_num=UART_NUM_1, .tx_gpio=17, .rx_gpio=16, .baud=115200, .rx_timeout_ms=300 };
    // up_init(&port, &cfg);

    // rb_init(&rb_rx, rb_rx_mem, sizeof(rb_rx_mem));
    // fc_init(&codec, payload_buf, sizeof(payload_buf));
    // ul_init(&link, &port, &rb_rx, &codec, 10, 10);

    // xTaskCreate(ul_uart_rx_task,"ul_rx",4096, &link, 10, NULL);
    // xTaskCreate(ul_frame_decode_task,"ul_dec",4096, &link, 9, NULL);
    // xTaskCreate(ul_uart_tx_task,"ul_tx",4096, &link, 9, NULL);

    // MsgRouter mr = { .on_ping = on_ping };
    // mr_run(&mr, &link); // never returns

    }
