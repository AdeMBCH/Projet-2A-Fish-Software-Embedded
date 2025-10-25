#ifndef MSG_ROUTER_H
#define MSG_ROUTER_H

#include <stdint.h>
#include "uart_link.h"

typedef void (*msg_handler_t)(const DecodedFrameMsg *m);

typedef struct {
    msg_handler_t on_text;
    msg_handler_t on_ping;
    msg_handler_t on_action;
    // add as needed
} MsgRouter;

void mr_run(MsgRouter *mr, UartLink *ul); // blocking loop

#endif
