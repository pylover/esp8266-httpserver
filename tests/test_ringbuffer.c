#include <stdlib.h>
#include <stdio.h>

#include "ringbuffer.h"


void report(RingBuffer *rb) {
	printf("used: %d, available: %d, h: %d, t: %d\r\n",
			rb_used(rb), rb_available(rb), rb->head, rb->tail);
}

#define S	10

int main() {
	char t[9];
	char *buffer = (char*)malloc(S);
	RingBuffer rb = {S, 0, 0, buffer};
	rb_push(&rb, "ABCD", 4);
	rb_push(&rb, "EFGHI", 5);
	rb_push(&rb, "JKLMN", 5);
	report(&rb);

	rb_pop(&rb, t, 9);
	printf("%s\r\n", t);
	report(&rb);
	

	free(buffer);

}
