#ifndef HTTPD_SESSION_H
#define HTTPD_SESSION_H

#include "common.h"
#include "tcpserver.h"


#define HTTPD_SESSION_GET(c) ({ \
    struct httpd_session * s= ((struct espconn *)c)->reverse; \
    s->conn = c; \
    s; })

#define HTTPD_SESSION_RESET(s) RB_RESET(&(s)->req_rb)



httpd_err_t httpd_session_init();
void httpd_session_deinit();
struct httpd_session * httpd_session_find(struct espconn *conn);
void httpd_session_delete(struct httpd_session *s);

#endif
