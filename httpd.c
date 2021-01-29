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


// TODO: Delete it
static struct httpd *server;


#define HTTP_RESPONSE_HEADER_FORMAT \
    "HTTP/1.1 %s\r\n" \
    "Server: lwIP/1.4.0\r\n" \
    "Expires: Fri, 10 Apr 2008 14:00:00 GMT\r\n" \
    "Pragma: no-cache\r\n" \
    "Content-Length: %d\r\n" \
    "Content-Type: %s\r\n" 


/* Request Start */

static ICACHE_FLASH_ATTR
struct httprequest * _findrequest(struct espconn *conn) {
    uint8_t i;
    struct httprequest *r; 
    esp_tcp *tcp = conn->proto.tcp;

    for (i = 0; i < HTTPD_MAXCONN; i++) {
        r = *(server->requests + i);
        if (r == NULL) {
            continue;
        }
        
        if ((memcmp(tcp->remote_ip, r->remote_ip, 4) == 0) &&
                (tcp->remote_port == r->remote_port)) {
            os_printf("Request Matched\n");
            r->conn = conn;
            return r;
        }
    }
    os_printf("Cannot found request: %d\n", i);
    return NULL;
}


// TODO: Document
// TODO: Error Handling
static ICACHE_FLASH_ATTR
int _deleterequest(struct httprequest *r, bool disconnect) {
    if (disconnect) {
        espconn_disconnect(r->conn);
    }
    *(server->requests + r->index) = NULL;
    os_free(r->headerbuff);
    os_free(r->respbuff);
    os_free(r);
}


static ICACHE_FLASH_ATTR
struct httprequest * _createrequest(struct espconn *conn) {
    /* Create and allocate a new request. */
    struct httprequest *r = os_zalloc(sizeof(struct httprequest));
    r->status = HRS_IDLE;
    
    /* Preserve IP and Port. */
    memcpy(conn->proto.tcp->remote_ip, r->remote_ip, 4);
    r->remote_port = conn->proto.tcp->remote_port;

    /* Allocate memory for header. */
    r->headerbuff = (char*)os_zalloc(HTTP_HEADER_BUFFER_SIZE);

    // TODO: Dynamic memory allocation for response buffer.
    r->respbuff = (char*)os_zalloc(HTTP_RESPONSE_BUFFER_SIZE);
    
    return r;
}


static ICACHE_FLASH_ATTR
err_t _ensurerequest(struct espconn *conn) {
    uint8_t i;
    struct httprequest *r;
    
    /* Find any pre-existing request. */
    r = _findrequest(conn);
    if (r != NULL) {
        /* Another dead request found, delete it. */
        os_printf("Another dead request found, delete it.\n");
        _deleterequest(r, false);
    }
    
    /* Finding a free slot in requests array. */
    for (i = 0; i < HTTPD_MAXCONN; i++) {
        r = *(server->requests + i);
        if (r == NULL) {
            /* Slot found, create and allocate a new request */
            r = _createrequest(conn);
            r->index = i;
            os_printf("New request: %d, %p\n", i, r);
            *(server->requests + i) = r;
            return HTTPD_OK;
        }
    }
    
    /* Raise Max connection error. */
    return HTTPD_MAXCONNEXCEED;
}

/* Request End */


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

    os_printf("Dispatching\n");
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
    
    struct httprequest *req = _findrequest(conn);
    //os_printf("Receive from "IPPORT_FMT".\r\n", remoteinfo(conn->proto.tcp));

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


// TODO: Document
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


// TODO: Document
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


// TODO: Document
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
    os_printf("HTTPD: client "IPPORT_FMT" err %d reconnecting...\r\n",  
            remoteinfo(conn->proto.tcp),
            err
        );
    struct httprequest *r = _findrequest(conn);
    if (r != NULL) {
        _deleterequest(r, true);
    }
}


static ICACHE_FLASH_ATTR
void _client_disconnected(void *arg) {
    struct espconn *conn = arg;
    os_printf("Client "IPPORT_FMT" has been disconnected.\r\n",  
            remoteinfo(conn->proto.tcp)
        );
    struct httprequest *r = _findrequest(conn);
    if (r != NULL) {
        _deleterequest(r, true);
    }
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
    s->requests = os_zalloc(sizeof(struct httprequest*) * HTTPD_MAXCONN);

    conn->type = ESPCONN_TCP;
    conn->state = ESPCONN_NONE;
    conn->proto.tcp = &s->esptcp;
    conn->proto.tcp->local_port = HTTPD_PORT;

    espconn_regist_connectcb(conn, _client_connected);
    espconn_set_opt(conn, ESPCONN_NODELAY);
    espconn_accept(conn);

    err = espconn_regist_time(conn, HTTPD_TIMEOUT, 0);
    if (err) {
        return HTTPD_INVALIDTIMEOUT;
    }
 
    err = espconn_tcp_set_max_con_allow(conn, HTTPD_MAXCONN);
    if (err) {
        return HTTPD_INVALIDMAXCONN;
    }
   
    server = s;

#ifdef HTTPD_VERBOSE
    os_printf(
        "HTTPD is listening on: "IPPORT_FMT"\r\n", localinfo(&s->esptcp)
    );
#endif

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
    int i; 
    struct httprequest **r = server->requests;

    err = espconn_disconnect(&s->connection);
    if (err) {
        return HTTPD_DISCONNECT;
    }

    err = espconn_delete(&s->connection);
    if (err) {
        return HTTPD_DELETECONNECTION;
    }
    
    for (i = 0; i < HTTPD_MAXCONN; i++) {
        r += i;
        _deleterequest(*r, true);
    }
    if (s->requests != NULL) {
        os_free(s->requests);
    }
   
    return HTTPD_OK;
}

