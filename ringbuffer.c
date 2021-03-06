#include "ringbuffer.h"

#include <osapi.h>


ICACHE_FLASH_ATTR
rberr_t rb_pushone(struct ringbuffer *b, char byte) {
    rb_size_t writernext = RB_WRITER_MOVE(b, 1);
    if (writernext == b->reader) {
        switch (b->overflow) {
            case RB_OVERFLOW_ERROR:
                return RB_ERR_INSUFFICIENT;
            case RB_OVERFLOW_IGNORE_OLDER:
                /* Ignore one byte and forward reader's needle one step */
                RB_READER_SKIP(b, 1);
                break;
            case RB_OVERFLOW_IGNORE_NEWER:
                /* Ignore the newly received byte */
                return RB_OK;
        }
    }

    b->blob[b->writer] = byte;
    b->writer = writernext;
    return RB_OK;
}


ICACHE_FLASH_ATTR
rberr_t rb_write(struct ringbuffer *b, char *data, rb_size_t len) {
    rb_size_t i;
    
    if ((b->overflow == RB_OVERFLOW_ERROR) && (RB_AVAILABLE(b) < len)) {
        return RB_ERR_INSUFFICIENT;
    }

    for(i = 0; i < len; i++) {
        rb_pushone(b, data[i]);
    }
    return RB_OK;
}


ICACHE_FLASH_ATTR
rb_size_t rb_read(struct ringbuffer *b, char *data, rb_size_t len) {
    rb_size_t i;
    for (i = 0; i < len; i++) {
        if (b->reader == b->writer) {
            return i;
        }
        data[i] = b->blob[b->reader];
        RB_READER_SKIP(b, 1);
    }
    return len;
}


ICACHE_FLASH_ATTR
rb_size_t rb_dryread(struct ringbuffer *b, char *data, rb_size_t len) {
    rb_size_t i;
    rb_size_t n;
    for (i = 0; i < len; i++) {
        n = RB_READER_MOVE(b, i);
        if (n == b->writer) {
            return i;
        }
        data[i] = b->blob[n];
    }
    return len;
}


ICACHE_FLASH_ATTR
rberr_t rb_read_until(struct ringbuffer *b, char *data, rb_size_t len,
        char *delimiter, rb_size_t dlen, rb_size_t *readlen) {
    rb_size_t i, n, mlen = 0;
    char tmp; 

    for (i = 0; i < len; i++) {
        n = RB_READER_MOVE(b, i);
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
void rb_init(struct ringbuffer *b, char *buff, rb_size_t size,
        enum rb_overflow overflow) {
    b->size = size;
    b->reader = 0;
    b->writer = 0;
    b->blob = buff;
    b->overflow = overflow;
}



