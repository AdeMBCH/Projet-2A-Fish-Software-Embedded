#include "frame_codec.h"

#define SOF 0x4A
static inline uint8_t crc8_xor_acc(uint8_t acc, uint8_t b){ return acc ^ b; }
static inline uint8_t crc8_xor_buf(uint8_t acc, const uint8_t *buf, int len){
    for(int i=0;i<len;i++) {
        acc ^= buf[i];
    }
    return acc;
}

void fc_init(FrameCodec *fc, uint8_t *buf, uint16_t sz){
    fc->payload_buf = buf; 
    fc->payload_buf_sz = sz; 
    fc_reset(fc);
}
void fc_reset(FrameCodec *fc){
    fc->st=FC_WAIT_SOF; 
    fc->func=0; 
    fc->len=0; 
    fc->idx=0; 
    fc->crc=0;
}

bool fc_push_byte(FrameCodec *fc, uint8_t b, Frame *out){
    switch(fc->st){
    case FC_WAIT_SOF:
        if(b==SOF){ 
            fc->st=FC_FUNC_H; 
            fc->crc=0; 
            fc->idx=0; 
            fc->func=0; 
            fc->len=0; 
        }
        break;

    case FC_FUNC_H:
        fc->func = ((uint16_t)b)<<8; 
        fc->crc = crc8_xor_acc(fc->crc, b); 
        fc->st=FC_FUNC_L; 
        break;

    case FC_FUNC_L:
        fc->func |= b; 
        fc->crc ^= b; 
        fc->st=FC_LEN_H; 
        break;

    case FC_LEN_H:
        fc->len = ((uint16_t)b)<<8; 
        fc->crc ^= b; 
        fc->st=FC_LEN_L; 
        break;

    case FC_LEN_L:
        fc->len |= b; fc->crc ^= b;
        if(fc->len>fc->payload_buf_sz){ 
            fc_reset(fc); 
            break; 
        }
        fc->idx=0; 
        fc->st = (fc->len==0)? FC_CRC : FC_PAYLOAD; 
        break;

    case FC_PAYLOAD:
        fc->payload_buf[fc->idx++] = b; 
        fc->crc ^= b;
        if(fc->idx==fc->len){
            fc->st=FC_CRC;
        }
        break;

    case FC_CRC:
        if(b==fc->crc){
            out->func = fc->func; 
            out->len = fc->len; 
            out->payload = fc->payload_buf; 
            out->crc=b;
            fc_reset(fc); 
            return true;
        } else { 
            fc_reset(fc); 
        }
        break;
    }
    return false;
}

int fc_encode(const Frame *in, uint8_t *out, int outsz){
    int need = 1+2+2+in->len+1; 
    if(outsz<need){
        return 0;
    }
    int i=0; 
    out[i++]=SOF;
    uint8_t crc=0;

    out[i]= (uint8_t)(in->func>>8); 
    crc^=out[i++]; 
    out[i]= (uint8_t)(in->func);    
    crc^=out[i++];
    out[i]= (uint8_t)(in->len>>8);  
    crc^=out[i++]; 
    out[i]= (uint8_t)(in->len);     
    crc^=out[i++];

    for(int k=0;k<in->len;k++){ 
        out[i]=in->payload[k]; 
        crc^=out[i++]; 
    }
    out[i++]=crc;
    return i;
}
