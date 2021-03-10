#ifndef SESSION_H
#define SESSION_H

#include "datamodel.h"


#define session_req_write(s, d, l) rb_write(&(s)->req_rb, (d), (l))
#define session_recv(s, d, l) rb_read(&(s)->req_rb, (d), (l))

#define session_recv_until(s, d, l, m, ml, ol) \
    rb_read_until(&(s)->req_rb, (d), (l), (m), (ml), (ol))

#define session_recv_until_chr(s, d, l, c, ol) \
    rb_read_until_chr(&(s)->req_rb, (d), (l), (c), (ol))

#define session_resp_write(s, d, l) rb_write(&(s)->resp_rb, (d), (l))
#define session_resp_read(s, d, l) rb_read(&(s)->resp_rb, (d), (l))

#define session_req_len(s) RB_USED(&(s)->req_rb)
#define session_resp_len(s) RB_USED(&(s)->resp_rb)

#define session_get(c) ({ \
    struct httpd_session * s= ((struct espconn *)c)->reverse; \
    s->conn = c; \
    s; })


httpd_err_t session_init();
void session_deinit();
struct httpd_session * session_find(struct espconn *conn);
void session_delete(struct httpd_session *s);

#endif
