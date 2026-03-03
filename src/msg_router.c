#include "msg_router.h"
#include <string.h>
#include "freertos/task.h"
#include "esp_log.h"


static void default_sink(const DecodedFrameMsg *m)
{
    (void)m;
}

/* Boucle bloquante : lit les messages décodés et appelle le bon handler */
void mr_run(MsgRouter *mr, UartLink *ul)
{
    DecodedFrameMsg m;
    for (;;) {
        if (xQueueReceive(ul->q_frames_rx, &m, portMAX_DELAY) == pdTRUE) {
            switch (m.func) {
            case UART_CMD_TEXT:
                if (mr->on_text) mr->on_text(&m);
                else default_sink(&m);
                break;
            case UART_CMD_PING:
                if (mr->on_ping) mr->on_ping(&m);
                break;
            case UART_CMD_ACTION:
                if (mr->on_action) mr->on_action(&m);
                break;
            case UART_CMD_SERVO_STOP:
                if (mr->app_on_servo_stop) mr->app_on_servo_stop(&m);
                break;

            case UART_CMD_SERVO_ENABLE:
                if (mr->app_on_servo_enable) mr->app_on_servo_enable(&m);
                break;
            case UART_CMD_SERVO_LEFT:
                if (mr->app_on_servo_left) mr->app_on_servo_left(&m);
                break;
            case UART_CMD_SERVO_RIGHT:
                if (mr->app_on_servo_right) mr->app_on_servo_right(&m);
                break;
            default:
                default_sink(&m);
                break;
            }
        }
        taskYIELD();
    }
}

void router_task(void *arg)
{
    RouterTaskArgs *a = (RouterTaskArgs *)arg;
    if (!a || !a->ul || !a->mr) {
        vTaskDelete(NULL);
    }
    mr_run(a->mr, a->ul);
}
