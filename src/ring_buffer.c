#include "ring_buffer.h"

#define MASK(i,s) ((i) & ((s)-1))

bool rb_init(RingBuffer *rb, uint8_t *storage, uint32_t size){
    if(!rb||!storage||size<8){
        return false;
    }

    rb->buf=storage; 
    rb->size=size; 
    rb->head=rb->tail=0; 
    return true;
}

uint32_t rb_used(const RingBuffer *rb){ 
    return rb->head - rb->tail; 
}
uint32_t rb_free(const RingBuffer *rb){ 
    return rb->size - rb_used(rb); 
}

bool rb_put(RingBuffer *rb, uint8_t b){
    if(rb_free(rb)==0){
        return false;
    }

    rb->buf[MASK(rb->head++, rb->size)] = b; 
    return true;
}

bool rb_get(RingBuffer *rb, uint8_t *b){
    if(rb_used(rb)==0){
        return false;
    }
    *b = rb->buf[MASK(rb->tail++, rb->size)]; 
    return true;
}

uint32_t rb_write(RingBuffer *rb, const uint8_t *data, uint32_t len){
    uint32_t w=0; 
    for(; w<len && rb_free(rb); ++w) {
        rb_put(rb, data[w]); 
    }
    return w;
}

uint32_t rb_read(RingBuffer *rb, uint8_t *data, uint32_t maxlen){
    uint32_t r=0; 
    for(; r<maxlen && rb_used(rb); ++r){
        rb_get(rb, &data[r]);
    } 
    return r;
}
