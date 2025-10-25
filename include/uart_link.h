#ifndef UART_LINK_H
#define UART_LINK_H

#include <stdint.h>
#include <stdbool.h>
#include "ring_buffer.h"
#include "frame_codec.h"
#include "uart_port.h"

typedef struct {
    UartPort *port;
    RingBuffer *rb_rx;
    FrameCodec *codec;
    QueueHandle_t q_frames_rx; // queue of Frame copies (small fixed struct + payload copy)
    QueueHandle_t q_frames_tx; // queue of Frame to send
    uint8_t enc_buf[600];      // encoded scratch
} UartLink;

typedef struct {
    uint16_t func;
    uint16_t len;
    uint8_t  data[512];
} DecodedFrameMsg;

bool ul_init(UartLink *ul, UartPort *port, RingBuffer *rb_rx, FrameCodec *codec, size_t qrx_depth, size_t qtx_depth);

// tasks
void ul_uart_rx_task(void *arg);
void ul_frame_decode_task(void *arg);
void ul_uart_tx_task(void *arg);

// public API
bool ul_send(UartLink *ul, uint16_t func, const uint8_t *payload, uint16_t len);

#endif