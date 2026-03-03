#ifndef MSG_ROUTER_H
#define MSG_ROUTER_H

#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "uart_link.h"
#include "uart_commands.h"

typedef void (*msg_handler_t)(const DecodedFrameMsg *m);

typedef struct {
    msg_handler_t on_text;
    msg_handler_t on_ping;
    msg_handler_t on_action;
    msg_handler_t app_on_servo_enable;
    msg_handler_t app_on_servo_stop;
    msg_handler_t app_on_servo_left;
    msg_handler_t app_on_servo_right;
    // Possible d'ajouter d'autres
} MsgRouter;

// Boucle principale du routeur : bloquante
void mr_run(MsgRouter *mr, UartLink *ul);


typedef struct {
    UartLink  *ul;
    MsgRouter *mr;
} RouterTaskArgs;

void router_task(void *arg);
#endif
