#ifndef tcpd_h
#define tcpd_h

#include "common.h"

#define TCPD_CLOSE(c) espconn_disconnect(c)

httpd_err_t tcpd_recv_hold(struct httpd_session *s);
httpd_err_t tcpd_recv_unhold(struct httpd_session *s);
void tcpd_print_espconn_err(err_t err);

#endif
