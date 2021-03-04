#ifndef SESSION_H
#define SESSION_H

#include "config.h"
#include "ringbuffer.h"

#include <c_types.h>
#include <ip_addr.h>
#include <espconn.h>

/**
 * Represents connected client.
 */
struct httpd_session{
    uint8_t remote_ip[4];
    uint16_t remote_port;

    // TODO: May be delte it
    struct espconn *conn;
    uint8_t id;

    char req_buff[HTTPD_REQ_BUFFSIZE];
    struct ringbuffer *req_rb;

    char resp_buff[HTTPD_RESP_BUFFSIZE];
    struct ringbuffer *resp_rb;
};


err_t session_init();
void session_deinit();
struct httpd_session * session_find(struct espconn *conn);

#endif
