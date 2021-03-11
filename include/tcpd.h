#ifndef tcpd_h
#define tcpd_h

#include "common.h"

#define tcpd_close(c) espconn_disconnect(c)
#define tcpd_recv_hold(s) espconn_recv_hold((s)->conn)
#define tcpd_recv_unhold(s) espconn_recv_unhold((s)->conn)

void tcpd_print_espconn_err(err_t err);

#endif
