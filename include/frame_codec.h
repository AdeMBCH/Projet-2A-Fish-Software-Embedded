#ifndef FRAME_CODEC_H
#define FRAME_CODEC_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint16_t func;
    uint16_t len;
    const uint8_t *payload; // TX view
    uint8_t  crc;
} Frame;

typedef struct {
    enum { FC_WAIT_SOF, FC_FUNC_H, FC_FUNC_L, FC_LEN_H, FC_LEN_L, FC_PAYLOAD, FC_CRC } st;
    uint16_t func, len;
    uint16_t idx;
    uint8_t  crc;
    uint8_t *payload_buf;
    uint16_t payload_buf_sz;
    uint32_t last_ok_cnt;
} FrameCodec;

void fc_init (FrameCodec *fc, uint8_t *payload_buf, uint16_t payload_buf_sz);
void fc_reset(FrameCodec *fc);
bool fc_push_byte(FrameCodec *fc, uint8_t b, Frame *out);   // true when a full valid frame is ready
int  fc_encode(const Frame *in, uint8_t *outbuf, int outsz);

#endif