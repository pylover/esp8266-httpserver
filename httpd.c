/**
 * @file
 * Lightweigth HTTP Server for microcontrollers.
 *
 */


#include "httpd.h"

#include <osapi.h>
#include <user_interface.h>
#include <mem.h>
#include <ets_sys.h>
#include <c_types.h>
#include <os_type.h>


struct httpd {
    struct espconn connection;
    esp_tcp esptcp;
    
    struct httprequest **requests;
    uint8_t requestscount;
    
    struct httproute *routes;
};



#define remotecmp(t, ip, p) \
    (memcmp((ip), (t)->remote_ip, 4) && ((p) != (t)->remote_port))


// TODO: Max connection

// TODO: Delete it
static struct httpd *server;


#define printrequest(r) os_printf("Connections: %d, "IPPORT_FMT"\r\n", \
        server->requestscount, \
        remoteinfo(r))
        

#define HTTP_RESPONSE_HEADER_FORMAT \
    "HTTP/1.1 %s\r\n" \
    "Server: lwIP/1.4.0\r\n" \
    "Expires: Fri, 10 Apr 2008 14:00:00 GMT\r\n" \
    "Pragma: no-cache\r\n" \
    "Content-Length: %d\r\n" \
    "Content-Type: %s\r\n" 


static ICACHE_FLASH_ATTR
struct httprequest * _findrequest(struct espconn *conn) {
    uint8_t i;
    struct httprequest *r;
    
    for (i = 0; i < HTTPD_MAXCONN; i++) {
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
int _deleterequest(struct httprequest *r, bool disconnect) {
    if (disconnect) {
        espconn_disconnect(r->conn);
    }
    server->requests[r->index] = NULL;
    server->requestscount--;
    os_free(r->headerbuff);
    os_free(r->respbuff);
    os_free(r);
}


static ICACHE_FLASH_ATTR
struct httprequest * _createrequest(struct espconn *conn) {
    /* Create and allocate a new request */
    struct httprequest *r = os_zalloc(sizeof(struct httprequest));
    r->status = HRS_IDLE;
    
    /* Allocate memory for header. */
    r->headerbuff = (char*)os_zalloc(HTTP_HEADER_BUFFER_SIZE);

    // TODO: Dynamic memory allocation for response buffer.
    r->respbuff = (char*)os_zalloc(HTTP_RESPONSE_BUFFER_SIZE);
    
    return r;
}


static ICACHE_FLASH_ATTR
int8_t _ensurerequest(struct espconn *conn) {
    uint8_t i;
    struct httprequest *r = _findrequest(conn);
    
    /* Find any pre-existing request. */
    if (r != NULL) {
        /* Another dead request found, delete it. */
        _disposerequest(r);
    }

    /* Find a free slot in requests array. */
    for (i = 0; i < HTTPD_MAXCONN; i++) {
        if (server->requests[i] == NULL) {
            /* Slot found, create and allocate a new request */
            server->requests[i] = _createrequest(conn);
            server->requestscount++;
            return i;
        }
    }
    
    /* Raise Max connection error. */
    return HTTPD_MAXCONNEXCEED;
}


static ICACHE_FLASH_ATTR
err_t httpd_send(struct httprequest *req, char *data, uint32_t length) {
    err_t err = espconn_send(req->conn, data, length);
    if ((err == ESPCONN_MEM) || (err == ESPCONN_MAXNUM)) {
        os_printf("TCP Send: Out of memory\r\n");
        return HTTPD_SENDMEMFULL;
    }
    
    if (err == ESPCONN_ARG) {
        os_printf("illegal argument; cannot find network transmission \
                according to structure espconn\r\n");

        _deleterequest(_findrequest(req->conn), true);
        return HTTPS_CONNECTIONLOST;
    }
    
    return HTTPD_OK;
}


static ICACHE_FLASH_ATTR
int _dispatch(struct httprequest *req, char *body, uint32_t body_length) {
    struct httproute *route = server->routes;
    int16_t statuscode;

    while (req->handler == NULL) {
        if (route->pattern == NULL){
            break;    
        }
        if (matchroute(route, req)) {
            req->handler = route->handler;
            os_printf("Route found: %s %s\r\n", route->verb, route->pattern);
            break;
        }
        route++;
    }
    
    if (req->handler == NULL) {
        os_printf("Not found: %s\r\n", req->path);
        return httpd_response_notfound(req);
    }
    
    bool last = (body_length - req->contentlength) == 2;
    uint32_t chunklen = last? body_length - 2: body_length;
    req->body_cursor += chunklen;
    uint32_t more = req->contentlength - req->body_cursor;
    ((Handler)req->handler)(req, body, chunklen, more);
}


static ICACHE_FLASH_ATTR
int _read_header(struct httprequest *req, char *data, uint16_t length) {
    // TODO: max header length check !
    char *cursor = os_strstr(data, "\r\n\r\n");
    char *headers;
    uint16_t content_type_len;

    uint16_t l = (cursor == NULL)? length: (cursor - data) + 4;
    os_memcpy(req->headerbuff + req->headerbuff_len, data, l);
    req->headerbuff_len += l;

    if (cursor == NULL) {
        cursor = os_strstr(data, "\r\n");
        if (cursor == NULL) {
            // Request for more data, incomplete http header
            return HTTPD_MOREDATA;
        }
    }
    
    req->verb = req->headerbuff;
    cursor = os_strchr(req->headerbuff, ' ');
    cursor[0] = 0;

    req->path = ++cursor;
    cursor = os_strchr(cursor, ' ');
    cursor[0] = 0;
    headers = cursor + 1;

    //cursor = os_strstr(headers, "Except: 100-continue");
    //if (cursor != NULL) {
    //    cursor = os_strstr(cursor, "\r\n");
    //    if (cursor == NULL) {
    //        return HTTPD_INVALIDEXCEPT;
    //    }
    //    return HTTPD_CONTINUE;
    //}

    req->contenttype = os_strstr(headers, "Content-Type:");
    if (req->contenttype != NULL) {
        cursor = os_strstr(req->contenttype, "\r\n");
        if (cursor == NULL) {
            return HTTPD_INVALIDCONTENTTYPE;
        }
        content_type_len = cursor - req->contenttype;
    }

    cursor = os_strstr(headers, "Content-Length:");
    if (cursor != NULL) {
        req->contentlength = atoi(cursor + 16);
        cursor = os_strstr(cursor, "\r\n");
        if (cursor == NULL) {
            return HTTPD_INVALIDCONTENTLENGTH;
        }
    }

    // Terminating strings
    if (req->contenttype != NULL) {
        req->contenttype[content_type_len] = 0;
    }
    return l;
}


static ICACHE_FLASH_ATTR
void _client_recv(void *arg, char *data, uint16_t length) {
    uint16_t remaining;
    int readsize;
    struct espconn *conn = arg;
    
    os_printf("Receive from "IPPORT_FMT".\r\n",  
            remoteinfo(conn->proto.tcp)
        );

    struct httprequest *req = _findrequest(conn);
    
    if (req->status < HRS_REQ_BODY) {
        req->status = HRS_REQ_HEADER;
        readsize = _read_header(req, data, length);
        if (readsize < 0) {
            os_printf("Invalid Header: %d\r\n", readsize);
            httpd_response_badrequest(req);
            // TODO: Close Connection
            return;
        }

        if (readsize == 0) {
            // Incomplete header
            os_printf("Incomplete Header: %d\r\n", readsize);
            httpd_response_badrequest(req);
            // TODO: Close Connection
            return;
        }

        remaining = length - readsize;
        os_printf("--> %s %s type: %s length: %d, remaining: %d-%d=%d\r\n", 
                req->verb,
                req->path,
                req->contenttype,
                req->contentlength,
                length,
                readsize,
                remaining
        );
        req->status = HRS_REQ_BODY;
    }
    else {
        remaining = length;
    }
    
    _dispatch(req, data + (length-remaining), remaining);
}


ICACHE_FLASH_ATTR
void httpd_response_start(struct httprequest *req, char *status, 
        char *contenttype, uint32_t contentlength, char **headers, 
        uint8_t headers_count) {
    int i;
    req->respbuff_len = os_sprintf(req->respbuff, 
            HTTP_RESPONSE_HEADER_FORMAT, status, contentlength, 
            contenttype);

    for (i = 0; i < headers_count; i++) {
        req->respbuff_len += os_sprintf(
                req->respbuff + req->respbuff_len, "%s\r\n", 
                headers[i]);
    }
    req->respbuff_len += os_sprintf(
            req->respbuff + req->respbuff_len, "\r\n");
}


ICACHE_FLASH_ATTR
err_t httpd_response_finalize(struct httprequest *req, char *body, uint32_t body_length) {
    err_t err;
    if (body_length > 0) {
        os_memcpy(req->respbuff + req->respbuff_len, body, 
                body_length);
        req->respbuff_len += body_length;
        req->respbuff_len += os_sprintf(
                req->respbuff + req->respbuff_len, "\r\n");
    }
    req->respbuff_len += os_sprintf(
            req->respbuff + req->respbuff_len, "\r\n");

    err = httpd_send(req, req->respbuff, req->respbuff_len);
    if (err) {
        return err;
    }
    _deleterequest(req, false);
    return HTTPD_OK;
}


ICACHE_FLASH_ATTR
err_t httpd_response(struct httprequest *req, char *status, char *contenttype, 
        char *content, uint32_t contentlength, char **headers, 
        uint8_t headers_count) {
    httpd_response_start(req, status, contenttype, contentlength, headers, 
            headers_count);
    return httpd_response_finalize(req, content, contentlength);
}


static ICACHE_FLASH_ATTR
void _client_recon(void *arg, int8_t err) {
    struct espconn *conn = arg;
    struct httprequest *r = _findrequest(conn);
    _deleterequest(r, true);
    os_printf("HTTPD: client "IPPORT_FMT" err %d reconnecting...\r\n",  
            remoteinfo(conn->proto.tcp),
            err
        );
}


static ICACHE_FLASH_ATTR
void _client_disconnected(void *arg) {
    struct espconn *conn = arg;
    struct httprequest *r = _findrequest(conn);
    _deleterequest(r, true);
    os_printf("Client "IPPORT_FMT" has been disconnected.\r\n",  
            remoteinfo(conn->proto.tcp)
        );
}


static ICACHE_FLASH_ATTR
void _client_connected(void *arg) {
    struct espconn *conn = arg;
    os_printf("Connected: "IPPORT_FMT".\r\n",  
            remoteinfo(conn->proto.tcp)
        );

    _ensurerequest(conn);
    espconn_regist_recvcb(conn, _client_recv);
    espconn_regist_reconcb(conn, _client_recon);
    espconn_regist_disconcb(conn, _client_disconnected);
}

/**
 * Initialize the HTTP Server
 * @param s httpd struct.
 * @param routes httproute struct array.
 * @return HTTPD_OK, HTTPD_INVALIDMAXCONN, HTTPD_INVALIDTIMEOUT.
 */
ICACHE_FLASH_ATTR 
err_t httpd_init(struct httpd *s, struct httproute *routes) {
    err_t err;
    struct espconn *conn = &s->connection;
    
    s->routes = routes;
    s->requestscount = 0;
    s->requests = os_zalloc(sizeof(struct httprequest*) * HTTPD_MAXCONN);

    conn->type = ESPCONN_TCP;
    conn->state = ESPCONN_NONE;
    conn->proto.tcp = &s->esptcp;
    conn->proto.tcp->local_port = HTTPD_PORT;

#ifdef HTTPD_VERBOSE
    os_printf(
        "HTTPD is listening on: "IPPORT_FMT"\r\n", localinfo(&s->esptcp)
    );
#endif

    espconn_regist_connectcb(conn, _client_connected);
    err = espconn_tcp_set_max_con_allow(conn, HTTPD_MAXCONN);
    if (err) {
        return HTTPD_INVALIDMAXCONN;
    }

    espconn_set_opt(conn, ESPCONN_NODELAY);
    espconn_accept(conn);
    err = espconn_regist_time(conn, HTTPD_TIMEOUT, 1);
    if (err) {
        return HTTPD_INVALIDTIMEOUT;
    }
    server = s;
    return OK;
}

/**
 * Stop and free all resources used by HTTP Server.
 *
 * Do not call this function in any espconn or httpd callback.
 * @param s httpd struct
 * @return HTTPD_OK, HTTPD_DISCONNECT, HTTPD_DELETECONNECTION
 */
ICACHE_FLASH_ATTR
err_t httpd_stop(struct httpd *s) {
    err_t err;
    
    err = espconn_disconnect(&s->connection);
    if (err) {
        return HTTPD_DISCONNECT;
    }

    err = espconn_delete(&s->connection);
    if (err) {
        return HTTPD_DELETECONNECTION;
    }

    if (s->requests != NULL) {
        os_free(s->requests);
    }
   
    return HTTPD_OK;
}

