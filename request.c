#include "request.h"


static ICACHE_FLASH_ATTR
HttpRequest * _findrequest(struct espconn *conn) {
    uint8_t i;
    HttpRequest *r;
    
    for (i = 0; i < HTTPSERVER_MAXCONN; i++) {
        r = server->requests[i];
        if (r == NULL) {
            continue;
        }
        if (remotecmp(conn->proto.tcp, r->remote_ip, r->remote_port) == 0) {
            r->conn = conn;
            return r;
        }
    }
    return NULL;
}

static ICACHE_FLASH_ATTR
int _deleterequest(HttpRequest *r, bool disconnect) {
    if (disconnect) {
        espconn_disconnect(r->conn);
    }
    server->requests[r->index] = NULL;
    os_free(r->headerbuff);
    os_free(r->respbuff);
    os_free(r);
}


static ICACHE_FLASH_ATTR
HttpRequest * _createrequest(struct espconn *conn, uint8_t index) {
    /* Create and allocate a new request */
    HttpRequest *r = os_zalloc(sizeof(HttpRequest));
    r->status = HRS_IDLE;
    server->requests[index] = r;
    
    /* Allocate memory for header. */
    r->headerbuff = (char*)os_zalloc(HTTP_HEADER_BUFFER_SIZE);

    // TODO: Dynamic memory allocation for response buffer.
    r->respbuff = (char*)os_zalloc(HTTP_RESPONSE_BUFFER_SIZE);
    
    return r;
}


static ICACHE_FLASH_ATTR
int8_t _ensurerequest(struct espconn *conn) {
    uint8_t i;
    HttpRequest *r = _findrequest(conn);
    
    /* Find any pre-existing request. */
    if (r != NULL) {
        /* Another dead request found, delete it. */
        _disposerequest(r);
    }

    /* Find a free slot in requests array. */
    for (i = 0; i < HTTPSERVER_MAXCONN; i++) {
        if (server->requests[i] == NULL) {
            /* Slot found, create and allocate a new request */
            server->requests[i] = _createrequest(conn, i);
            return i;
        }
    }
    
    /* Raise Max connection error. */
    return ERR_MAXCONN;
}


