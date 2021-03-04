#include "config.h"
#include "common.h"
#include "session.h"
#include "tcpd.h"
#include "taskq.h"

#include <osapi.h>


static ICACHE_FLASH_ATTR
void _recv_cb(void *arg, char *data, uint16_t len) {
    struct httpd_session *session = session_get(arg); 

    rberr_t err = session_feed_req(session, data, len);
    if (err) {
        /* Buffer full, dispose session */
        taskq_push(HTTPD_SIG_CLOSE, session);
        return;
    }
    DEBUG("TCP RECV "IPPSTR"\r\n", IPP2STR(session));
    taskq_push(HTTPD_SIG_RECV, session);
}


static ICACHE_FLASH_ATTR
void _sent_cb(void *arg) {
    struct espconn *conn = arg;
    // TODO: post task send
    DEBUG("TCP SENT "IPPSTR".\r\n", IPP2STR(conn->proto.tcp));
}


static ICACHE_FLASH_ATTR
void _reconnect_cb(void *arg, int8_t err) {
    struct espconn *conn = arg;
    struct httpd_session *session = conn->reverse;
    // TODO: post task delete connection
    os_printf("TCP RECONN "IPPSTR" err %d reconnecting...\r\n",  
            //IPP2STR(conn->proto.tcp),
            IPP2STR(session),
            err
        );
}


static ICACHE_FLASH_ATTR
void _disconnect_cb(void *arg) {
    struct espconn *conn = arg;
    // TODO: post task delete connection
    INFO("TCP "IPPSTR" has been disconnected.\r\n", IPP2STR(conn->proto.tcp));
}

/**
 * Client connected
 */
static ICACHE_FLASH_ATTR
void _connect_cb(void *arg)
{
    struct espconn *conn = arg;
    struct httpd_session *session;
    err_t err = session_create(conn, &session);
    if(err) {
        DEBUG("Error creating session: %d"CR, err);
        taskq_push(HTTPD_SIG_REJECT, conn);
        return;
    }
    INFO("Session "IPPSTR" has been connected.\r\n", IPP2STR(session));
    espconn_regist_recvcb(conn, _recv_cb);
    espconn_regist_sentcb(conn, _sent_cb);
    espconn_regist_disconcb(conn, _disconnect_cb);
}

/**
 * Initialize tcp server connection.
 */
err_t tcpd_init(struct espconn *conn) {
    static esp_tcp _esptcp;
    err_t err;

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
    espconn_delete(conn);
}


void tcpd_print_err(err_t err) {
    
    char msg[256];
    switch (err) {
        case ESPCONN_ARG:
            os_sprintf(msg, "Illegal argument, cannot find TCP connection"
                    "according to structure espconn."CR);
            break;

        case ESPCONN_MEM: 
            os_sprintf(msg, "Out of memory"CR);
            break;

        case ESPCONN_ISCONN:
            os_sprintf(msg, "Already connected"CR);
            break;

        case ESPCONN_INPROGRESS: 
            os_sprintf(msg, "the connection is still in progress; please call"
                "espconn_disconnect to disconnect before deleting it."CR);
    }
    DEBUG(msg);
}


