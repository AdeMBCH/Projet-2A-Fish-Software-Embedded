#include "uart_link.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include <string.h>

bool ul_init(UartLink *ul, UartPort *port, RingBuffer *rb_rx, FrameCodec *codec, size_t qrx_depth, size_t qtx_depth){
    ul->port=port; 
    ul->rb_rx=rb_rx; 
    ul->codec=codec;
    ul->q_frames_rx = xQueueCreate(qrx_depth, sizeof(DecodedFrameMsg));
    ul->q_frames_tx = xQueueCreate(qtx_depth, sizeof(DecodedFrameMsg));

    return ul->q_frames_rx && ul->q_frames_tx;
}

void ul_uart_rx_task(void *arg){
    UartLink *ul=(UartLink*)arg; 
    uint8_t tmp[256];

    for(;;){
        int n = up_read_bytes(ul->port, tmp, sizeof(tmp), 20);
        if(n>0) {
            rb_write(ul->rb_rx, tmp, n);
        }
    }
}

void ul_frame_decode_task(void *arg){
    UartLink *ul=(UartLink*)arg;
    uint8_t b; 
    Frame f; 
    DecodedFrameMsg m;

    for(;;){
        if(rb_get(ul->rb_rx,&b)){
            if(fc_push_byte(ul->codec,b,&f)){
                m.func=f.func; 
                m.len=f.len;
                if(m.len>sizeof(m.data)) {
                    m.len=sizeof(m.data);
                }
                memcpy(m.data, f.payload, m.len);
                xQueueSend(ul->q_frames_rx, &m, 0);
            }
        } else {
            vTaskDelay(pdMS_TO_TICKS(2));
        }
    }
}

bool ul_send(UartLink *ul, uint16_t func, const uint8_t *payload, uint16_t len){
    DecodedFrameMsg m = {.func=func, .len=len};
    if(len>sizeof(m.data)){
        return false;
    }
    if(payload && len) {
        memcpy(m.data, payload, len);
    }
    return xQueueSend(ul->q_frames_tx, &m, 0)==pdTRUE;
}

void ul_uart_tx_task(void *arg){
    UartLink *ul=(UartLink*)arg; DecodedFrameMsg m;
    for(;;){
        if(xQueueReceive(ul->q_frames_tx,&m,portMAX_DELAY)==pdTRUE){
            Frame f = {.func=m.func,.len=m.len,.payload=m.data};
            int n = fc_encode(&f, ul->enc_buf, sizeof(ul->enc_buf));
            if(n>0){
                up_write_bytes(ul->port, ul->enc_buf, n);
            }
        }
    }
}
