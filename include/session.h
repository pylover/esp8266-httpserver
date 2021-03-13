#ifndef SESSION_H
#define SESSION_H

#include "common.h"
#include "tcpd.h"


#define HTTPD_SESSION_GET(c) ({ \
    struct httpd_session * s= ((struct espconn *)c)->reverse; \
    s->conn = c; \
    s; })


httpd_err_t session_init();
void session_deinit();
struct httpd_session * session_find(struct espconn *conn);
void session_delete(struct httpd_session *s);

#endif
