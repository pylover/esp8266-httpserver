#include "httpd.h"
#include "common.h"

#include <osapi.h>
#include <mem.h>


static struct httpd *server;


static ICACHE_FLASH_ATTR
void _recv_cb(void *arg, char *data, uint16_t length) {
    struct espconn *conn = arg;
    os_printf("TCP RECV "IPPSTR"\r\n", IPP2STR(conn->proto.tcp));
    espconn_send(conn, data, length);
}

static ICACHE_FLASH_ATTR
void _sent_cb(void *arg) {
    struct espconn *conn = arg;
    os_printf("TCP SENT "IPPSTR".\r\n", IPP2STR(conn->proto.tcp));
}

static ICACHE_FLASH_ATTR
void _reconnect_cb(void *arg, int8_t err) {
    struct espconn *conn = arg;
    os_printf("TCP RECONN "IPPSTR" err %d reconnecting...\r\n",  
            IPP2STR(conn->proto.tcp),
            err
        );
}


static ICACHE_FLASH_ATTR
void _disconnect_cb(void *arg) {
    struct espconn *conn = arg;
    os_printf("TCP "IPPSTR" has been disconnected.\r\n",  
            IPP2STR(conn->proto.tcp)
        );
}


static ICACHE_FLASH_ATTR
void _connect_cb(void *arg)
{
    struct espconn *conn = arg;
    espconn_regist_recvcb(conn, _recv_cb);
    espconn_regist_sentcb(conn, _sent_cb);
    espconn_regist_disconcb(conn, _disconnect_cb);
}


ICACHE_FLASH_ATTR 
err_t httpd_init() {
    server = (struct httpd*) os_zalloc(sizeof(struct httpd));
    server->connection.type = ESPCONN_TCP;
    server->connection.state = ESPCONN_NONE;
    server->connection.proto.tcp = &server->esptcp;
    server->connection.proto.tcp->local_port = HTTPD_PORT;
    os_printf("TCP Server is listening on: "IPPSTR"\r\n",  
            IPP2STR_LOCAL(server->connection.proto.tcp)
    );

    espconn_regist_connectcb(&server->connection, _connect_cb);
    espconn_regist_reconcb(&server->connection, _reconnect_cb);
    espconn_tcp_set_max_con_allow(&server->connection, HTTPD_MAXCONN);

    /*
     * ESPCONN_START = 0x00,
     * ESPCONN_REUSEADDR = 0x01,
     * ESPCONN_NODELAY = 0x02,
     * ESPCONN_COPY = 0x04,
     * ESPCONN_KEEPALIVE = 0x08,
     * ESPCONN_END
     */
    //espconn_set_opt(&server->connection, ESPCONN_NODELAY);

    espconn_accept(&server->connection);
    espconn_regist_time(&server->connection, HTTPD_TIMEOUT, 0);
    return OK;
}


ICACHE_FLASH_ATTR
void httpd_deinit() {
    // TODO: Do not call me from any espconn callback
    espconn_disconnect(&server->connection);
    espconn_delete(&server->connection);
    os_free(server);
}

