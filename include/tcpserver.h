#ifndef HTTPD_TCP_H
#define HTTPD_TCP_H

#include "common.h"

#define TCPD_CLOSE(c) espconn_disconnect(c)

httpd_err_t httpd_tcp_recv_hold(struct httpd_session *s);
httpd_err_t httpd_tcp_recv_unhold(struct httpd_session *s);
void httpd_tcp_print_err(err_t err);

httpd_err_t httpd_tcp_init(struct espconn *conn);
void httpd_tcp_deinit(struct espconn *conn);

#endif
