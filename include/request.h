#ifndef REQUEST_H
#define REQUEST_H

#include "common.h"

#define HTTPD_REQ_WRITE(s, d, l) rb_write(&(s)->req_rb, (d), (l))
#define HTTPD_RECV(s, d, l) rb_read(&(s)->req_rb, (d), (l))
#define HTTPD_DRYRECV(s, d, l) rb_dryread(&(s)->req_rb, (d), (l))
#define HTTPD_RECV_SKIP(s, l) RB_READER_SKIP(&(s)->req_rb, (l))
#define HTTPD_RECV_UNTIL(s, d, l, m, ml, ol) \
    rb_read_until(&(s)->req_rb, (d), (l), (m), (ml), (ol))

#define HTTPD_DRYRECV_UNTIL(s, d, l, m, ml, ol) \
    rb_read_until(&(s)->req_rb, (d), (l), (m), (ml), (ol))

#define HTTPD_RECV_UNTIL_CHR(s, d, l, c, ol) \
    rb_read_until_chr(&(s)->req_rb, (d), (l), (c), (ol))


#define HTTPD_REQ_AVAILABLE(s) RB_AVAILABLE(&(s)->req_rb)
#define HTTPD_REQ_LEN(s) RB_USED(&(s)->req_rb)


#define HTTPD_REQUESTBODY_REMAINING(s) \
    MAX(0, ((int)(s)->request.contentlen) - ((int)(s)->req_rb.writecounter))

httpd_err_t http_request_parse(struct httpd_session *s);

#endif
