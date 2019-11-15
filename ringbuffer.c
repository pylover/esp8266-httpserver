#include <osapi.h>
#include <mem.h>

#include "ringbuffer.h"


ICACHE_FLASH_ATTR
void rb_pushone(RingBuffer *rb, char byte) {
	//os_printf("inc tail\r\n", rb->tail, byte);
	//os_printf("inc tail:%d  %d\r\n", rb->tail, byte);
	rb->blob[rb->tail] = byte;
	//os_printf("inc tail Done:%d  %d\r\n", rb->tail, byte);
	rb_increment(rb, rb->tail, 1);
	if (rb->tail == rb->head) {
		rb_increment(rb, rb->head, 1);
	}
}


ICACHE_FLASH_ATTR
void rb_push(RingBuffer *rb, char *data, Size datalen) {
	Size i;
	for(i = 0; i < datalen; i++) {
		rb_pushone(rb, data[i]);
	}
	rb->blob[rb->tail] = '\0';
}


ICACHE_FLASH_ATTR
void rb_pop(RingBuffer *rb, char *data, Size datalen) {
	Size i;
	for (i = 0; i < datalen; i++) {
		data[i] = rb->blob[rb->head];
		rb_increment(rb, rb->head, 1); 
	}
}


ICACHE_FLASH_ATTR
int rb_safepush(RingBuffer *rb, char *data, Size datalen) {
	if (rb_canpush(rb, datalen)) {
		rb_push(rb, data, datalen);
		return RB_OK;
	}
	return RB_FULL;
}


ICACHE_FLASH_ATTR
int rb_safepop(RingBuffer *rb, char *data, Size datalen) {
	if (rb_canpop(rb, datalen)) {
		rb_pop(rb, data, datalen);
		return RB_OK;
	}
	return RB_INSUFFICIENT;
}


ICACHE_FLASH_ATTR
void rb_drypop(RingBuffer *rb, char *data, Size datalen) {
	Size i;
	for (i = 0; i < datalen; i++) {
		data[i] = rb->blob[rb_calc(rb, rb->head, i)];
	}
}


ICACHE_FLASH_ATTR
void rb_reset(RingBuffer *rb) {
	rb->head = 0;
	rb->tail = 0;
}



