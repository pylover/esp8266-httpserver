#include "ringbuffer.h"

#include <osapi.h>

#define MIN(x, y) (((x) > (y))? (y): (x))

/* Return count in buffer.  */
#define CIRC_CNT(writer,reader,size) (((writer) - (reader)) & ((size)-1))

/* Return space available, 0..size-1.  We always leave one free char
   as a completely full buffer has writer == reader, which is the same as
   empty.  */
#define CIRC_SPACE(writer,reader,size) CIRC_CNT((reader),((writer)+1),(size))

/* Return count up to the end of the buffer.  Carefully avoid
   accessing writer and reader more than once, so they can change
   underneath us without returning inconsistent results.  */
#define CIRC_CNT_TO_END(b) \
	({int end = ((b)->size) - ((b)->reader); \
	  int n = (((b)->writer) + end) & (((b)->size)-1); \
	  n < end ? n : end;})

/* Return space available up to the end of the buffer.  */
#define CIRC_SPACE_TO_END(b) \
	({int end = ((b)->size) - 1 - ((b)->writer); \
	  int n = (end + ((b)->reader)) & (((b)->size)-1); \
	  n <= end ? n : end+1;})


ICACHE_FLASH_ATTR
httpd_err_t rb_write(struct ringbuffer *b, char *data, size16_t len) {
    
    if (RB_AVAILABLE(b) < len) {
        return RB_ERR_INSUFFICIENT;
    }
    
    size16_t chunklen = MIN(CIRC_SPACE_TO_END(b), len);
    os_memcpy(b->blob + b->writer, data, chunklen);
    b->writer = RB_WRITER_CALC(b, chunklen);
    b->writecounter += chunklen;

    if (len > chunklen) {
        len -= chunklen;
        os_memcpy(b->blob, data + chunklen, len);
        b->writer += len;
        b->writecounter += len;
    }
    return RB_OK;
}


ICACHE_FLASH_ATTR
size16_t rb_read(struct ringbuffer *b, char *data, size16_t len) {
    size16_t total = MIN(CIRC_CNT_TO_END(b), len);
    os_memcpy(data, b->blob + b->reader, total);
    b->reader = RB_READER_CALC(b, total);
    len -= total;

    if (len) {
        len = MIN(CIRC_CNT_TO_END(b), len);
        os_memcpy(data + total, b->blob, len);
        b->reader += len;
        total += len;
    }
   
    return total;
}


ICACHE_FLASH_ATTR
size16_t rb_dryread(struct ringbuffer *b, char *data, size16_t len) {
    size16_t reader, total = MIN(CIRC_CNT_TO_END(b), len);
    os_memcpy(data, b->blob + b->reader, total);
    reader = RB_READER_CALC(b, total);
    len -= total;

    if (len) {
        len = MIN(RB_USED(b) - total, len);
        os_memcpy(data + total, b->blob, len);
        total += len;
    }
    return total;
}


ICACHE_FLASH_ATTR
httpd_err_t rb_read_until(struct ringbuffer *b, char *data, size16_t len,
        char *delimiter, size16_t dlen, size16_t *readlen) {
    size16_t i, n, mlen = 0;
    char tmp; 

    for (i = 0; i < len; i++) {
        n = RB_READER_CALC(b, i);
        if (n == b->writer) {
            return RB_ERR_NOTFOUND;
        }
        tmp = b->blob[n];
        data[i] = tmp;
        if (tmp == delimiter[mlen]) {
           mlen++;
           if (mlen == dlen) {
               *readlen = i + 1;
               RB_READER_SKIP(b, *readlen);
               return RB_OK;
           }
        }
        else if (mlen > 0) {
            mlen = 0;
        }
    }
    return RB_ERR_NOTFOUND;
}


ICACHE_FLASH_ATTR
httpd_err_t rb_dryread_until(struct ringbuffer *b, char *data, size16_t len,
        char *delimiter, size16_t dlen, size16_t *readlen) {
    size16_t i, n, mlen = 0;
    char tmp; 

    for (i = 0; i < len; i++) {
        n = RB_READER_CALC(b, i);
        if (n == b->writer) {
            return RB_ERR_NOTFOUND;
        }
        tmp = b->blob[n];
        data[i] = tmp;
        if (tmp == delimiter[mlen]) {
           mlen++;
           if (mlen == dlen) {
               *readlen = i + 1;
               return RB_OK;
           }
        }
        else if (mlen > 0) {
            mlen = 0;
        }
    }
    return RB_ERR_NOTFOUND;
}


ICACHE_FLASH_ATTR
httpd_err_t rb_read_until_chr(struct ringbuffer *b, char *data, size16_t len,
        char delimiter, size16_t *readlen) {
    size16_t i, n;
    char tmp; 

    for (i = 0; i < len; i++) {
        n = RB_READER_CALC(b, i);
        if (n == b->writer) {
            return RB_ERR_NOTFOUND;
        }
        tmp = b->blob[n];
        data[i] = tmp;
        if (tmp == delimiter) {
            *readlen = i + 1;
            RB_READER_SKIP(b, *readlen);
            return RB_OK;
        }
    }
    return RB_ERR_NOTFOUND;
}


ICACHE_FLASH_ATTR
void rb_init(struct ringbuffer *b, char *buff, size16_t size,
        enum rb_overflow overflow) {
    b->size = size;
    b->reader = 0;
    b->writer = 0;
    b->writecounter = 0;
    b->blob = buff;
    b->overflow = overflow;
}



