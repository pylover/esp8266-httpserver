#ifndef tcpd_h
#define tcpd_h

#include "common.h"

#define TCPD_CLOSE(c) espconn_disconnect(c)
#define TCPD_RECV_HOLD(s) espconn_recv_hold((s)->conn)

void tcpd_print_espconn_err(err_t err);

#endif
