#include "tcpserver.h"
#include "request.h"
#include "session.h"


static ICACHE_FLASH_ATTR
void _recv_cb(void *arg, char *data, uint16_t len) {
    struct httpd_session *s = HTTPD_SESSION_GET(arg); 
    httpd_err_t err;
    
    err = httpd_tcp_recv_hold(s);
    if (err) {
        httpd_tcp_print_err(err);
        return;
    }
    if (s->status == HTTPD_SESSIONSTATUS_CLOSING) {
        /* Closing sessino: Ignore the received data. */
        return;
    }
    err = HTTPD_REQ_WRITE(s, data, len);
    if (err) {
        /* Buffer full, dispose session. */
        HTTPD_SCHEDULE(HTTPD_SIG_CLOSE, s);
        return;
    }
    
    /* call httpd recv. */
    httpd_recv(s);
}


static ICACHE_FLASH_ATTR
void _sent_cb(void *arg) {
    struct httpd_session *s= HTTPD_SESSION_GET(arg); 
    HTTPD_SCHEDULE(HTTPD_SIG_SEND, s);
}


static ICACHE_FLASH_ATTR
void _reconnect_cb(void *arg, int8_t err) {
    struct espconn *conn = arg;
    struct httpd_session *s = HTTPD_SESSION_GET(conn);
    INFO("TCP RECONN "IPPSTR" err %d.", IPP2STR(s), err);
    httpd_session_delete(s);
}


static ICACHE_FLASH_ATTR
void _disconnect_cb(void *arg) {
    struct espconn *conn = arg;
    /* disconn cb Find session */
    struct httpd_session *s= HTTPD_SESSION_GET(conn);
    INFO("TCP "IPPSTR" has been disconnected.", IPP2STR(conn->proto.tcp));
    /* Session: %p id: %d */
    if (s) {
        httpd_session_delete(s);
    }
}

/**
 * Client connected
 */
static ICACHE_FLASH_ATTR
void _connect_cb(void *arg) {
    struct espconn *conn = arg;
    struct httpd_session *s;
    httpd_err_t err = httpd_session_create(conn, &s);
    if(err) {
        ERROR("Error creating session: %d", err);
        HTTPD_SCHEDULE(HTTPD_SIG_REJECT, conn);
        return;
    }
    INFO("Client "IPPSTR" has been connected.", IPP2STR(conn->proto.tcp));
    espconn_regist_recvcb(conn, _recv_cb);
    espconn_regist_sentcb(conn, _sent_cb);
    espconn_regist_disconcb(conn, _disconnect_cb);
}

httpd_err_t httpd_tcp_recv_hold(struct httpd_session *s) {
    httpd_err_t err;

    /* hold Recv: conn state: %p. */
    if (s->status == HTTPD_SESSIONSTATUS_IDLE) {
        /* Hold Recv. */
        err = espconn_recv_hold(s->conn);
        if (err) {
            httpd_tcp_print_err(err);
            return;
        }
        s->status = HTTPD_SESSIONSTATUS_RECVHOLD;
    }
    return HTTPD_OK;
}


httpd_err_t httpd_tcp_recv_unhold(struct httpd_session *s) {
    httpd_err_t err;

    /* Unhold Recv: conn state: %p. */
    if (s->status != HTTPD_SESSIONSTATUS_RECVHOLD) {
        return HTTPD_OK;
    }
    err = espconn_recv_unhold(s->conn);
    if (err) {
        return err;
    }
    s->status = HTTPD_SESSIONSTATUS_IDLE;
    return HTTPD_OK;
}



void httpd_tcp_print_err(err_t err) {
    switch (err) {
        case ESPCONN_ARG:
            ERROR("Illegal argument, cannot find TCP connectionaccording to "
                "structure espconn.");
            break;
        case ESPCONN_RTE: 
            ERROR("Routing Problem");
            break;

        case ESPCONN_MEM: 
            ERROR("Out of memory");
            break;
        case ESPCONN_MAXNUM:
            ERROR("buffer (or 8 packets at most) of sending data is full.");
            break;
        case ESPCONN_ISCONN:
            ERROR("Already connected");
            break;

        case ESPCONN_INPROGRESS: 
            ERROR("the connection is still in progress; please call"
                "espconn_disconnect to disconnect before deleting it.");
            break;
        case ESPCONN_IF: 
            ERROR("Fail to send UDP data.");
            break;
        deafult:
            ERROR("Unknown error: %d", err);
            break;
    }
}

/**
 * Initialize tcp server connection.
 */
httpd_err_t httpd_tcp_init(struct espconn *conn) {
    static esp_tcp _esptcp;
    httpd_err_t err;

    conn->type = ESPCONN_TCP;
    conn->state = ESPCONN_NONE;
    conn->proto.tcp = &_esptcp;
    conn->proto.tcp->local_port = HTTPD_PORT;

    espconn_regist_connectcb(conn, _connect_cb);
    espconn_regist_reconcb(conn, _reconnect_cb);
    espconn_tcp_set_max_con_allow(conn, HTTPD_MAXCONN);

    /*
     * ESPCONN_START = 0x00,
     * ESPCONN_REUSEADDR = 0x01,
     * ESPCONN_NODELAY = 0x02,
     * ESPCONN_COPY = 0x04,
     * ESPCONN_KEEPALIVE = 0x08,
     * ESPCONN_END
     */
    espconn_set_opt(conn, ESPCONN_NODELAY);

    /*
     * enum espconn_level{
     *    ESPCONN_KEEPIDLE,
     *    ESPCONN_KEEPINTVL,
     *    ESPCONN_KEEPCNT
     * }
     */
    //espconn_set_keepalive(&server->connection, level, optarg)
    
    err = espconn_accept(conn);
    if (err) {
        return err;
    }
    espconn_regist_time(conn, HTTPD_TIMEOUT, 0);
    return HTTPD_OK;
}

/**
 *
 */
void httpd_tcp_deinit(struct espconn *conn) {
    httpd_err_t err;
    err = espconn_delete(conn);
    if (err) {
        httpd_tcp_print_err(err);
    }
}


