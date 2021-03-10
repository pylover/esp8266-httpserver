#include "ringbuffer.h"


ICACHE_FLASH_ATTR
httpd_err_t rb_pushone(struct ringbuffer *b, char byte) {
    size16_t writernext = RB_WRITER_CALC(b, 1);
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
                b->writecounter++;
                return RB_OK;
        }
    }

    b->blob[b->writer] = byte;
    b->writer = writernext;
    b->writecounter++;
    return RB_OK;
}


ICACHE_FLASH_ATTR
httpd_err_t rb_write(struct ringbuffer *b, char *data, size16_t len) {
    size16_t i;
    
    if ((b->overflow == RB_OVERFLOW_ERROR) && (RB_AVAILABLE(b) < len)) {
        return RB_ERR_INSUFFICIENT;
    }

    for(i = 0; i < len; i++) {
        rb_pushone(b, data[i]);
    }
    return RB_OK;
}


ICACHE_FLASH_ATTR
size16_t rb_read(struct ringbuffer *b, char *data, size16_t len) {
    size16_t i;
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
size16_t rb_dryread(struct ringbuffer *b, char *data, size16_t len) {
    size16_t i;
    size16_t n;
    for (i = 0; i < len; i++) {
        n = RB_READER_CALC(b, i);
        if (n == b->writer) {
            return i;
        }
        data[i] = b->blob[n];
    }
    return len;
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



