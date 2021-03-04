#include "session.h"
#include "common.h"

#include <mem.h>


static struct httpd_session **sessions;


ICACHE_FLASH_ATTR
struct httpd_session * session_find(struct espconn *conn) {
    uint8_t i;
    esp_tcp *tcp = conn->proto.tcp;
    struct httpd_session *s;

    for (i = 0; i < HTTPD_MAXCONN; i++) {
        s = *(sessions + i);
        if (s == NULL) {
            continue;
        }
        
        if ((memcmp(tcp->remote_ip, s->remote_ip, 4) == 0) &&
                (tcp->remote_port == s->remote_port)) {
            s->conn = conn;
            return s;
        }
    }
    return NULL;
}


ICACHE_FLASH_ATTR
void session_delete(struct httpd_session *s, bool disconnect) {
    if (disconnect) {
        espconn_disconnect(s->conn);
    }
    *(sessions + s->id) = NULL;
    os_free(s);
}

/**
 * Create, allocate and store request.
 */
ICACHE_FLASH_ATTR
err_t session_create(struct espconn *conn, struct httpd_session **out) {
    size_t i;
    struct httpd_session *s;
    
    /* Find any pre-existing dead client. */
    s = session_find(conn);
    if (s != NULL) {
        /* Another dead request found, delete it. */
        session_delete(s, false);
    }

    /* Finding a free slot in requests array. */
    for (i = 0; i < HTTPD_MAXCONN; i++) {
        s = *(sessions + i);
        if (s == NULL) {
            /* Slot found */
            break;
        }
    }
    
    /* No free slot found. Raise Max connection error. */
    if (i == HTTPD_MAXCONN) {
        return HTTPD_ERR_MAXCONNEXCEED;
    }

    /* Create and allocate a new session. */
    s = (struct httpd_session *)os_zalloc(sizeof(struct httpd_session));
    
    /* Preserve IP and Port. */
    memcpy(s->remote_ip, conn->proto.tcp->remote_ip, 4);
    s->remote_port = conn->proto.tcp->remote_port;
    
    /* Initialize ringbuffers. */
    rb_init(s->req_rb, s->req_buff, HTTPD_REQ_BUFFSIZE, RB_OVERFLOW_ERROR);
    rb_init(s->resp_rb, s->resp_buff, HTTPD_RESP_BUFFSIZE, RB_OVERFLOW_ERROR);

    os_printf("Session created "IPPSTR"."CR, IPP2STR(s));
    s->id = i;
    *(sessions + i) = s;
    if (out) {
        *out = s;
    }
    return HTTPD_OK;
}


ICACHE_FLASH_ATTR
err_t session_init() {
    sessions = (struct httpd_session**) 
        os_zalloc(sizeof(struct httpd_session*) * HTTPD_MAXCONN);
    
    if (sessions == NULL) {
        return HTTPD_ERR_MEMFULL;
    }
    return HTTPD_OK;
}


ICACHE_FLASH_ATTR
void session_deinit() {
    // TODO: Close and delete all clients/connections
    os_free(sessions);
}
