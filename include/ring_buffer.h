#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint8_t *buf;
    uint32_t size;     // power of two recommended
    volatile uint32_t head;
    volatile uint32_t tail;
} RingBuffer;


bool rb_init(RingBuffer *rb, uint8_t *storage, uint32_t size);
uint32_t rb_write(RingBuffer *rb, const uint8_t *data, uint32_t len);
uint32_t rb_read (RingBuffer *rb, uint8_t *data, uint32_t maxlen);
uint32_t rb_used (const RingBuffer *rb);
uint32_t rb_free (const RingBuffer *rb);
bool rb_put  (RingBuffer *rb, uint8_t b);
bool rb_get  (RingBuffer *rb, uint8_t *b);

#endif