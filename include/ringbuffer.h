#ifndef RINGBUFFER_H
#define RINGBUFFER_H

#include "common.h"

typedef struct {
	Size size;
	int head;
	int tail;
	char * blob;
} RingBuffer;


void rb_pushone(RingBuffer *rb, char byte);
void rb_push(RingBuffer *rb, char *data, Size datalen);
void rb_popone(RingBuffer *rb, char *data);
void rb_pop(RingBuffer *rb, char *data, Size datalen);
void rb_drypop(RingBuffer *rb, char *data, Size datalen);
void rb_skip(RingBuffer *rb, Size datalen);
int rb_safepush(RingBuffer *rb, char *data, Size datalen);
int rb_safepop(RingBuffer *rb, char *data, Size datalen);
void rb_reset(RingBuffer *rb);

#define rb_calc(rb, i, s) ((s + i) % rb->size)
#define rb_increment(rb, i, s) i = rb_calc(rb, i, s)
#define rb_skip(rb, s) rb_increment(rb, rb->head, s)
#define rb_used(rb) ((rb->head > rb->tail? rb->size: 0) + rb->tail - rb->head)
#define rb_available(rb) (rb->size - rb_used(rb) - 1)
#define rb_canpush(rb, len) (len <= rb_available(rb))
#define rb_canpop(rb, len) (len <= rb_used(rb))


typedef enum {
	RB_OK = 0,
	RB_FULL,
	RB_INSUFFICIENT
} RingBufferError;

#endif

