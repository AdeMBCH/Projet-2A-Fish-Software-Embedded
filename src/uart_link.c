#include "uart_link.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include <string.h>
#include "esp_log.h"

bool ul_init(UartLink *ul, UartPort *port, RingBuffer *rb_rx, FrameCodec *codec, size_t qrx_depth, size_t qtx_depth){
    ul->port=port; 
    ul->rb_rx=rb_rx; 
    ul->codec=codec;
    ul->q_frames_rx = xQueueCreate(qrx_depth, sizeof(DecodedFrameMsg));
    ul->q_frames_tx = xQueueCreate(qtx_depth, sizeof(DecodedFrameMsg));
    return ul->q_frames_rx && ul->q_frames_tx;
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

void ul_uart_rx_task(void *arg){
    UartLink *ul = (UartLink*)arg;
    UartPort *up = ul->port;
    uart_event_t evt;
    uint8_t tmp[256];

    for(;;){
        if (xQueueReceive(up->evt_q, &evt, portMAX_DELAY) != pdTRUE) continue;

        switch(evt.type){
        case UART_DATA: {
            int to_read = evt.size;
            while (to_read > 0){
                int chunk = to_read > (int)sizeof(tmp) ? (int)sizeof(tmp) : to_read;
                int n = up_read_bytes(up, tmp, chunk, 0);
                if (n <= 0) break;
                rb_write(ul->rb_rx, tmp, (uint32_t)n);
                to_read -= n;
            }
            if (ul->dec_task) xTaskNotifyGive(ul->dec_task);
        } break;

        case UART_FIFO_OVF:
        case UART_BUFFER_FULL:
            // Purge driver et prévenir décodeur d’un trou éventuel
            uart_flush_input(up->cfg.uart_num);
            if (ul->dec_task) xTaskNotifyGive(ul->dec_task);
            break;

        default:
            break;
        }
    }
}

void ul_frame_decode_task(void *arg){
    UartLink *ul = (UartLink*)arg;
    ul->dec_task = xTaskGetCurrentTaskHandle();

    uint8_t b;
    Frame f;
    DecodedFrameMsg m;
    const int BUDGET = 512;

    for(;;){
        // Dort jusqu’à notification d’arrivée de données
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        // Draine avec budget pour laisser tourner IDLE0
        int processed = 0;
        while (rb_get(ul->rb_rx, &b)){
            if (fc_push_byte(ul->codec, b, &f)){
                m.func = f.func;
                m.len  = f.len > sizeof(m.data) ? sizeof(m.data) : f.len;
                if (m.len) memcpy(m.data, f.payload, m.len);
                xQueueSend(ul->q_frames_rx, &m, 0);
            }
            if (++processed >= BUDGET){
                processed = 0;
                taskYIELD();
            }
        }
    }
}

void ul_uart_tx_task(void *arg){
    UartLink *ul=(UartLink*)arg; DecodedFrameMsg m;
    for(;;){
        if (xQueueReceive(ul->q_frames_tx,&m,portMAX_DELAY)==pdTRUE){
            Frame f = {.func=m.func,.len=m.len,.payload=m.data};
            int n = fc_encode(&f, ul->enc_buf, sizeof(ul->enc_buf));
            if (n>0) up_write_bytes(ul->port, ul->enc_buf, n);
        }
    }
}
