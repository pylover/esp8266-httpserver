#include "session.h"


static struct httpd_session **sessions;




ICACHE_FLASH_ATTR
void session_reset(struct httpd_session *s) {
    // TODO: init resp_rb too, if needed
    RB_RESET(&s->req_rb);
}


ICACHE_FLASH_ATTR
httpd_err_t session_send(struct httpd_session *s, char * data, size16_t len) {
    httpd_err_t err;
    char tmp[HTTPD_CHUNK];
    size16_t tmplen;
    if ((data != NULL) && (len > 0)) {
        httpd_err_t err = rb_write(&s->resp_rb, data, len);
        if (err) {
            return err;
        }
    }

    tmplen = session_resp_dryread(s, tmp, HTTPD_CHUNK);
    /* Reading data from response buffer to send: %u */
    if (tmplen <= 0) {
        if (s->status == HTTPD_SESSIONSTATUS_CLOSING) {
            err = session_close(s);
            if (err) {
                return err;
            }
            s->status = HTTPD_SESSIONSTATUS_CLOSED;
        }
        return HTTPD_OK;
    }
    
    /* espconn_send: %d */
    err = espconn_send(s->conn, tmp, tmplen);
    if (err == ESPCONN_MAXNUM) {
        /* send buffer is full. shedule it. */
        if (!HTTPD_SCHEDULE(HTTPD_SIG_SEND, s)) {
            ERROR("Cannot push task, queue is full.");
            return HTTPD_ERR_TASKQ_FULL;
        }
        return HTTPD_OK;
    }
    else if (err) {
        return err;
    }
    
    session_resp_skip(s, tmplen);
    return HTTPD_OK;
}


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
void session_delete(struct httpd_session *s) {
    *(sessions + s->id) = NULL;
    os_free(s);
}

/**
 * Create, allocate and store request.
 */
ICACHE_FLASH_ATTR
httpd_err_t session_create(struct espconn *conn, struct httpd_session **out) {
    size_t i;
    struct httpd_session *s;
    
    /* Find any pre-existing dead client. */
    s = session_find(conn);
    if (s != NULL) {
        DEBUG("Dead session found.");
        /* Another dead request found, delete it. */
        session_delete(s);
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
    s->status = HTTPD_SESSIONSTATUS_IDLE;
    s->conn = conn;
    conn->reverse = s;

    /* Initialize ringbuffers. */
    rb_init(&s->req_rb, s->req_buff, HTTPD_REQ_BUFFSIZE, RB_OVERFLOW_ERROR);
    rb_init(&s->resp_rb, s->resp_buff, HTTPD_RESP_BUFFSIZE, RB_OVERFLOW_ERROR);

    INFO("Session created "IPPSTR".", IPP2STR(s));
    s->id = i;
    *(sessions + i) = s;
    if (out) {
        *out = s;
    }
    return HTTPD_OK;
}


ICACHE_FLASH_ATTR
httpd_err_t session_init() {
    sessions = (struct httpd_session**) 
        os_zalloc(sizeof(struct httpd_session*) * HTTPD_MAXCONN);
    
    if (sessions == NULL) {
        return HTTPD_ERR_MEMFULL;
    }
    return HTTPD_OK;
}


ICACHE_FLASH_ATTR
void session_deinit() {
    uint8_t i;
    struct httpd_session *s;

    for (i = 0; i < HTTPD_MAXCONN; i++) {
        s = *(sessions + i);
        if (s == NULL) {
            continue;
        }
        HTTPD_SCHEDULE(HTTPD_SIG_CLOSE, s);
    }
    os_free(sessions);
}
