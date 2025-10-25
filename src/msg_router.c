#include "msg_router.h"
#include "uart_commands.h"

static void default_sink(const DecodedFrameMsg *m){ 
    (void)m; 
}

void mr_run(MsgRouter *mr, UartLink *ul){
    DecodedFrameMsg m;
    for(;;){
        if(xQueueReceive(ul->q_frames_rx,&m, portMAX_DELAY)==pdTRUE){
            switch(m.func){
            case UART_CMD_TEXT:   
                (mr->on_text?mr->on_text:&default_sink)(&m); 
                break;
            case UART_CMD_PING:   
                if(mr->on_ping) {
                    mr->on_ping(&m);
                }
                break;
            case UART_CMD_ACTION: 
                if(mr->on_action) {
                    mr->on_action(&m); 
                }
                break;
            default: 
                default_sink(&m);
                break;
            }
        }
    }
}
