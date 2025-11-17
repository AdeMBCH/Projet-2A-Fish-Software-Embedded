#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "priority.h"
#include "uart_port.h"
#include "ring_buffer.h"
#include "frame_codec.h"
#include "uart_link.h"
#include "msg_router.h"
#include "uart_commands.h"

#include "servo_ctrl.h"
#include "tofm.h"

#define RB_RX_SZ   2048
#define PAYLOAD_SZ 512

static UartPort port;
static UartLink link;
static RingBuffer rb_rx;
static uint8_t    rb_rx_mem[RB_RX_SZ];
static FrameCodec codec;
static uint8_t    payload_buf[PAYLOAD_SZ];

static ServoCtrl stail;
static ServoCtrl stail2;

static MsgRouter mr;
static RouterTaskArgs rargs;

static void on_ping(const DecodedFrameMsg *m){
    (void)m;
    const char pong[] = "pong";
    ul_send(&link, UART_CMD_PONG, (const uint8_t*)pong, sizeof(pong)-1);
}

 void app_main(void){
    // //Servo
    sc_init(&stail, 18);
    st_set_us_limits(&stail.model, 1000,1500,2000);
    sc_set_profile(&stail, 1.0f, 30.f, 0.f);
    sc_start(&stail);

    //Test servo 2
    sc_init(&stail2, 19);
    st_set_us_limits(&stail2.model, 1000,1500,2000);
    sc_set_profile(&stail2, 0.1f, 60.f, 0.f);
    sc_start(&stail2);

    //UART
    UartPortCfg cfg={ .uart_num=UART_NUM_2, .tx_gpio=17, .rx_gpio=16, .baud=115200, .rx_timeout_ms=50 };
    up_init(&port,&cfg);
    rb_init(&rb_rx, rb_rx_mem, sizeof(rb_rx_mem));
    fc_init(&codec, payload_buf, sizeof(payload_buf));
    ul_init(&link, &port, &rb_rx, &codec, 32, 32);
    
    //Router
    mr.on_ping  = on_ping;
    mr.on_text  = NULL;
    mr.on_action = NULL;
    rargs.ul = &link;
    rargs.mr = &mr;

    // Tâches

    xTaskCreatePinnedToCore(tof_task, "tof_task", 4096, NULL, PRIO_CTRL, NULL, COMCORE);
    // xTaskCreatePinnedToCore(servo_task, "servo_tasktest", 3072, &stail2, PRIO_CTRL, NULL, SERVOCORE);
    // xTaskCreatePinnedToCore(servo_task, "servo_task", 3072, &stail, PRIO_CTRL, NULL, SERVOCORE);
    // xTaskCreatePinnedToCore(ul_uart_rx_task,     "ul_rx", 4096, &link, PRIO_UART_RX, NULL, COMCORE);
    // xTaskCreatePinnedToCore(ul_frame_decode_task,"ul_dec",4096, &link, PRIO_DEC,     NULL, COMCORE);
    // xTaskCreatePinnedToCore(ul_uart_tx_task,     "ul_tx", 4096, &link, PRIO_UART_TX, NULL, COMCORE);
    // xTaskCreatePinnedToCore(router_task,         "router",4096, &rargs,PRIO_ROUTER,  NULL, COMCORE);

}