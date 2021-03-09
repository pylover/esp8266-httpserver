#include "config.h"
#include "common.h"
#include "session.h"
#include "tcpd.h"
#include "taskq.h"

#include <osapi.h>
#include <ip_addr.h>
#include <espconn.h>


static ICACHE_FLASH_ATTR
void _recv_cb(void *arg, char *data, uint16_t len) {
    struct httpd_session *session = session_get(arg); 

    httpd_err_t err = session_req_write(session, data, len);
    if (err) {
        /* Buffer full, dispose session */
        taskq_push(HTTPD_SIG_CLOSE, session);
        return;
    }
    taskq_push(HTTPD_SIG_RECV, session);
}


static ICACHE_FLASH_ATTR
void _sent_cb(void *arg) {
    struct httpd_session *session = session_get(arg); 
    taskq_push(HTTPD_SIG_SEND, session);
}


static ICACHE_FLASH_ATTR
void _reconnect_cb(void *arg, int8_t err) {
    struct espconn *conn = arg;
    struct httpd_session *session = session_get(conn);
    INFO("TCP RECONN "IPPSTR" err %d."CR, IPP2STR(session), err);
    session_delete(session);
}


static ICACHE_FLASH_ATTR
void _disconnect_cb(void *arg) {
    struct espconn *conn = arg;
    struct httpd_session *session = session_get(conn);
    INFO("TCP "IPPSTR" has been disconnected."CR, IPP2STR(conn->proto.tcp));
    session_delete(session);
}

/**
 * Client connected
 */
static ICACHE_FLASH_ATTR
void _connect_cb(void *arg) {
    struct espconn *conn = arg;
    struct httpd_session *session;
    INFO("Client "IPPSTR" has been connected."CR, IPP2STR(session));
    httpd_err_t err = session_create(conn, &session);
    if(err) {
        ERROR("Error creating session: %d"CR, err);
        taskq_push(HTTPD_SIG_REJECT, conn);
        return;
    }
    espconn_regist_recvcb(conn, _recv_cb);
    espconn_regist_sentcb(conn, _sent_cb);
    espconn_regist_disconcb(conn, _disconnect_cb);
}


httpd_err_t tcpd_reject(struct espconn *conn) {
    return espconn_disconnect(conn);
}


httpd_err_t tcpd_close(struct httpd_session *s) {
    return espconn_disconnect(s->conn);
}


void tcpd_print_err(httpd_err_t err) {
    #define MSG(s, ... ) os_sprintf(msg, s CR, ## __VA_ARGS__ )

    char msg[256];
    switch (err) {
        case ESPCONN_ARG:
            MSG("Illegal argument, cannot find TCP connectionaccording to "
                "structure espconn.");
            break;
        case ESPCONN_RTE: 
            MSG("Routing Problem");
            break;

        case ESPCONN_MEM: 
            MSG("Out of memory");
            break;
        case ESPCONN_MAXNUM:
            MSG("buffer (or 8 packets at most) of sending data is full.");
            break;
        case ESPCONN_ISCONN:
            MSG("Already connected");
            break;

        case ESPCONN_INPROGRESS: 
            MSG("the connection is still in progress; please call"
                "espconn_disconnect to disconnect before deleting it.");
            break;
        case ESPCONN_IF: 
            MSG("Fail to send UDP data.");
            break;
        deafult:
            MSG("Unknown error: %d", err);
            break;

    }
    ERROR(msg);
}

/**
 * Initialize tcp server connection.
 */
httpd_err_t tcpd_init(struct espconn *conn) {
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
    //espconn_set_opt(&server->connection, ESPCONN_NODELAY);

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
void tcpd_deinit(struct espconn *conn) {
    httpd_err_t err;
    err = espconn_delete(conn);
    tcpd_print_err(err);
}


