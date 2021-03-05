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
    uint8_t id;
    struct espconn *conn;

    uint8_t remote_ip[4];
    uint16_t remote_port;

    char req_buff[HTTPD_REQ_BUFFSIZE];
    struct ringbuffer req_rb;

    char resp_buff[HTTPD_RESP_BUFFSIZE];
    struct ringbuffer resp_rb;
};


#define session_req_write(s, d, l) rb_write(&(s)->req_rb, (d), (l))
#define session_recv(s, d, l) rb_read(&(s)->req_rb, (d), (l))
#define session_resp_read(s, d, l) rb_read(&(s)->resp_rb, (d), (l))

#define session_req_len(s) RB_USED(&(s)->req_rb)
#define session_resp_len(s) RB_USED(&(s)->resp_rb)

#define session_get(c) ({ \
    struct httpd_session * s= ((struct espconn *)c)->reverse; \
    s->conn = c; \
    s; })


err_t session_init();
void session_deinit();
struct httpd_session * session_find(struct espconn *conn);
void session_delete(struct httpd_session *s);

#endif
