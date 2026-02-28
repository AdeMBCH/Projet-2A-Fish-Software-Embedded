#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "priority.h"
#include "uart_port.h"
#include "ring_buffer.h"
#include "frame_codec.h"
#include "uart_link.h"
#include "msg_router.h"
#include "uart_commands.h"
#include "fish_motion.h"
#include "fish_fsm.h"

#include "servo_ctrl.h"
#include "tofm.h"

#define RB_RX_SZ   2048
#define PAYLOAD_SZ 512 //512 par défaut

static UartPort port;
static UartLink link;
static RingBuffer rb_rx;
static uint8_t    rb_rx_mem[RB_RX_SZ];
static FrameCodec codec;
static uint8_t    payload_buf[PAYLOAD_SZ];

static ServoCtrl stail;
// static ServoCtrl stail2;

static MsgRouter mr;
static RouterTaskArgs rargs;

// static ServoCtrl g_servo_tail;
// static FishCtrl g_fish_ctrl;

static void on_ping(const DecodedFrameMsg *m){
    (void)m;
    const char pong[] = "pong";
    ul_send(&link, UART_CMD_PONG, (const uint8_t*)pong, sizeof(pong)-1);
}

/* Commandes venant de la Raspberry via UART */

static void app_on_servo_stop(const DecodedFrameMsg *m)
{
    (void)m;
    sc_stop(&stail);
}

static void app_on_servo_enable(const DecodedFrameMsg *m)
{
    (void)m;
    sc_set_profile(&stail, 6.0f, 45.f, 0.f);
    sc_start(&stail);
}


 void app_main(void){
    //Servo
    sc_init(&stail, 19);
    st_set_us_limits(&stail.model, 1000,1500,2000);
    // sc_set_profile(&stail, 2.0f, 60.f, 0.f);
    // sc_start(&stail);

    // //Test servo 2
    // sc_init(&stail2, 19);
    // st_set_us_limits(&stail2.model, 1000,1500,2000);
    // sc_set_profile(&stail2, 0.1f, 60.f, 0.f);
    // sc_start(&stail2);

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
    mr.app_on_servo_enable = app_on_servo_enable;
    mr.app_on_servo_stop = app_on_servo_stop;
    rargs.ul = &link;
    rargs.mr = &mr;

    //Tâches

    // xTaskCreatePinnedToCore(tof_task, "tof_task", 4096, NULL, PRIO_CTRL, NULL, COMCORE);
    // xTaskCreatePinnedToCore(servo_task, "servo_tasktest", 3072, &stail2, PRIO_CTRL, NULL, SERVOCORE);
    xTaskCreatePinnedToCore(servo_task, "servo_task", 3072, &stail, PRIO_CTRL, NULL, SERVOCORE);
    // xTaskCreatePinnedToCore(ul_uart_rx_task,     "ul_rx", 4096, &link, PRIO_UART_RX, NULL, COMCORE);
    // xTaskCreatePinnedToCore(ul_frame_decode_task,"ul_dec",4096, &link, PRIO_DEC,     NULL, COMCORE);
    // xTaskCreatePinnedToCore(ul_uart_tx_task,     "ul_tx", 4096, &link, PRIO_UART_TX, NULL, COMCORE);
    // xTaskCreatePinnedToCore(router_task,         "router",4096, &rargs,PRIO_ROUTER,  NULL, COMCORE);

    // //UART
    // UartPortCfg cfg={ .uart_num=UART_NUM_2, .tx_gpio=17, .rx_gpio=16, .baud=115200, .rx_timeout_ms=50 };
    // up_init(&port,&cfg);
    // rb_init(&rb_rx, rb_rx_mem, sizeof(rb_rx_mem));
    // fc_init(&codec, payload_buf, sizeof(payload_buf));
    // ul_init(&link, &port, &rb_rx, &codec, 32, 32);
    
    // //Router
    // mr.on_ping  = on_ping;
    // mr.on_text  = NULL;
    // mr.on_action = NULL;
    // rargs.ul = &link;
    // rargs.mr = &mr;

    // // Init ToF (tâche asynchrone lira en continu)
    // tof_init();

    // // Init servo queue (GPIO à adapter)
    // sc_init(&g_servo_tail, /* gpio tail */ 18);

    // // Initialisation de la machine à états + mouvement
    // // params = NULL -> valeurs par défaut (voir robot_motion.c)
    // fish_ctrl_init(&g_fish_ctrl, &g_servo_tail, NULL);

    // // Tâches
    // xTaskCreatePinnedToCore(tof_task,"tof_task",4096, NULL, PRIO_CTRL, NULL, COMCORE);

    // xTaskCreatePinnedToCore(servo_task,    "servo_task",
    //                         3072, &g_servo_tail, PRIO_CTRL, NULL, SERVOCORE);

    // xTaskCreatePinnedToCore(fish_ctrl_task, "fish_ctrl",
    //                         4096, &g_fish_ctrl, PRIO_CTRL, NULL, COMCORE);

    // Com
    xTaskCreatePinnedToCore(ul_uart_rx_task,     "ul_rx",  4096, &link, PRIO_UART_RX, NULL, COMCORE);
    xTaskCreatePinnedToCore(ul_frame_decode_task,"ul_dec", 4096, &link, PRIO_DEC,     NULL, COMCORE);
    xTaskCreatePinnedToCore(ul_uart_tx_task,     "ul_tx",  4096, &link, PRIO_UART_TX, NULL, COMCORE);
    xTaskCreatePinnedToCore(router_task,         "router", 4096, &rargs,PRIO_ROUTER,  NULL, COMCORE);
}