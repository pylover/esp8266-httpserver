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


#define INFO( format, ... ) os_printf( format, ## __VA_ARGS__ )

static struct httpd *server;


// TODO: Use reverse instead of _findrequest


#define HTTP_RESPONSE_HEADER_FORMAT \
    HTTPVER" %s\r\n" \
    "Server: lwIP/1.4.0\r\n" \
    "Expires: Fri, 10 Apr 2008 14:00:00 GMT\r\n" \
    "Pragma: no-cache\r\n" \
    "Content-Length: %d\r\n" \
    "Content-Type: %s\r\n" 


/* Request Start */

static ICACHE_FLASH_ATTR
struct httpd_request * _findrequest(struct espconn *conn) {
    uint8_t i;
    struct httpd_request *r; 
    esp_tcp *tcp = conn->proto.tcp;

    for (i = 0; i < HTTPD_MAXCONN; i++) {
        r = *(server->requests + i);
        if (r == NULL) {
            continue;
        }
        
        if ((memcmp(tcp->remote_ip, r->remote_ip, 4) == 0) &&
                (tcp->remote_port == r->remote_port)) {
            r->conn = conn;
            return r;
        }
    }
    return NULL;
}


static ICACHE_FLASH_ATTR
void _deleterequest(struct httpd_request *r, bool disconnect) {
    if (disconnect) {
        espconn_disconnect(r->conn);
    }
    *(server->requests + r->index) = NULL;
    os_free(r->req_headerbuff);
    os_free(r->resp_headerbuff);
    os_free(r->resp_buff);
    os_free(r);
}


static ICACHE_FLASH_ATTR
struct httpd_request * _createrequest(struct espconn *conn) {
    /* Create and allocate a new request. */
    struct httpd_request *r = os_zalloc(sizeof(struct httpd_request));
    r->status = HRS_IDLE;
    
    /* Preserve IP and Port. */
    memcpy(r->remote_ip, conn->proto.tcp->remote_ip, 4);
    r->remote_port = conn->proto.tcp->remote_port;

    os_printf("Request Created "IPPORT_FMT"\r\n", remoteinfo(r));

    /* Allocate memory for header. */
    r->req_headerbuff = (char*)os_zalloc(HTTP_REQUESTHEADER_BUFFERSIZE);

    // TODO: Dynamic memory allocation for response buffer.
    r->resp_headerbuff = (char*)os_zalloc(HTTP_RESPONSEHEADER_BUFFERSIZE);
    r->resp_buff = (char*)os_zalloc(HTTP_RESPONSE_BUFFERSIZE);
    
    return r;
}


static ICACHE_FLASH_ATTR
err_t _ensurerequest(struct espconn *conn, struct httpd_request **req) {
    uint8_t i;
    struct httpd_request *r;
    
    /* Find any pre-existing request. */
    r = _findrequest(conn);
    if (r != NULL) {
        /* Another dead request found, delete it. */
        _deleterequest(r, false);
    }
    
    /* Finding a free slot in requests array. */
    for (i = 0; i < HTTPD_MAXCONN; i++) {
        r = *(server->requests + i);
        if (r == NULL) {
            /* Slot found, create and allocate a new request */
            r = _createrequest(conn);
            r->index = i;
            *(server->requests + i) = r;
            *req = r;
            return HTTPD_OK;
        }
    }
    
    /* Raise Max connection error. */
    return HTTPD_MAXCONNEXCEED;
}

/* Request End */


static ICACHE_FLASH_ATTR
err_t httpd_send(struct httpd_request *req, char *data, uint32_t length) {
    err_t err = espconn_send(req->conn, data, length);
    if ((err == ESPCONN_MEM) || (err == ESPCONN_MAXNUM)) {
        os_printf("Send mem full\n");
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
int _dispatch(struct httpd_request *req, char *body, uint32_t body_length) {
    struct httproute *route = server->routes;
    int16_t statuscode;

    while (req->handler == NULL) {
        if (route->pattern == NULL){
            break;    
        }
        if (matchroute(route, req)) {
            req->handler = route->handler;
            //os_printf("Route found: %s %s\r\n", route->verb, route->pattern);
            break;
        }
        route++;
    }
    
    if (req->handler == NULL) {
        os_printf("Route not found: %s %s\r\n", req->verb, req->path);
        return httpd_response_notfound(req);
    }
    
    bool last = (body_length - req->contentlength) == 2;
    uint32_t chunklen = last? body_length - 2: body_length;
    req->body_cursor += chunklen;
    uint32_t more = req->contentlength - req->body_cursor;
    ((Handler)req->handler)(req, body, chunklen, more);
}


static ICACHE_FLASH_ATTR
int _read_header(struct httpd_request *req, char *data, uint16_t length) {
    // TODO: max header length check !
    char *cursor = os_strstr(data, "\r\n\r\n");
    char *headers;
    uint16_t content_type_len;

    int l = (cursor == NULL)? length: (cursor - data) + 4;
    os_memcpy(req->req_headerbuff + req->req_headerbuff_len, data, l);
    req->req_headerbuff_len += l;

    if (cursor == NULL) {
        cursor = os_strstr(data, "\r\n");
        if (cursor == NULL) {
            // Request for more data, incomplete http header
            return HTTPD_MOREDATA;
        }
    }
    
    req->verb = req->req_headerbuff;
    cursor = os_strchr(req->req_headerbuff, ' ');
    cursor[0] = 0;

    req->path = ++cursor;
    cursor = os_strchr(cursor, ' ');
    cursor[0] = 0;
    headers = cursor + 1;

    req->contenttype = strcasestr(headers, "Content-Type:");
    if (req->contenttype != NULL) {
        cursor = os_strstr(req->contenttype, "\r\n");
        if (cursor == NULL) {
            return HTTPD_INVALIDCONTENTTYPE;
        }
        content_type_len = cursor - req->contenttype;
    }

    cursor = strcasestr(headers, "Content-Length:");
    if (cursor != NULL) {
        req->contentlength = atoi(cursor + 16);
        cursor = os_strstr(cursor, "\r\n");
        if (cursor == NULL) {
            l = HTTPD_INVALIDCONTENTLENGTH;
            goto finish;
        }
    }
    
    // Expect: 100-continue
    cursor = strcasestr(headers, "expect: 100-continue");
    if (cursor != NULL) {
        cursor = os_strstr(cursor, "\r\n");
        if (cursor == NULL) {
            l = HTTPD_INVALIDEXCEPT;
            goto finish;
        }
        l = HTTPD_CONTINUE;
        goto finish;
    }

finish:
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
    struct httpd_request *req = _findrequest(conn);
    //os_printf("Receive from "IPPORT_FMT".\r\n", remoteinfo(conn->proto.tcp));

    if (req->status < HRS_REQ_BODY) {
        req->status = HRS_REQ_HEADER;
        readsize = _read_header(req, data, length);
        //INFO("Read header: %d bytes\n", readsize);
        if (readsize < 0) {
            if (readsize == HTTPD_CONTINUE) {
                req->status = HRS_REQ_BODY;
                httpd_response_continue(req);
                return;
            }
            os_printf("Invalid Header: %d\r\n", readsize);
            httpd_response_badrequest(req);
            return;
        }
            
        if (readsize == 0) {
            // Incomplete header
            os_printf("Incomplete Header: %d\r\n", readsize);
            httpd_response_badrequest(req);
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


static ICACHE_FLASH_ATTR
void _send_response_body(void *arg) {
    err_t err;
    struct espconn *conn = arg;
    if (conn->reverse == NULL) {
        return;
    }
    struct httpd_request *r = conn->reverse;
    //os_printf("Sending body to "IPPORT_FMT"...\r\n", remoteinfo(r));

    /* send request body */
    err = httpd_send(r, r->resp_buff, r->resp_buff_len);
    if (err) {
        return err;
    }
    conn->reverse = NULL;
    //os_printf("Body Sent\n");
    _deleterequest(r, false);
}


ICACHE_FLASH_ATTR
void httpd_response_start(struct httpd_request *req, char *status, 
        char *contenttype, uint32_t contentlength, char **headers, 
        uint8_t headers_count) {
    int i;
    req->resp_headerbuff_len = os_sprintf(req->resp_headerbuff, 
            HTTP_RESPONSE_HEADER_FORMAT, status, contentlength, 
            contenttype);

    for (i = 0; i < headers_count; i++) {
        req->resp_headerbuff_len += os_sprintf(
                req->resp_headerbuff + req->resp_headerbuff_len, "%s\r\n", 
                headers[i]);
    }
    req->resp_headerbuff_len += os_sprintf(
            req->resp_headerbuff + req->resp_headerbuff_len, "\r\n");
}


ICACHE_FLASH_ATTR
err_t httpd_response_finalize(struct httpd_request *req, char *body, 
        uint32_t body_length) {
    err_t err;
    if (body_length > 0) {
        os_memcpy(req->resp_buff, body, body_length);
        req->resp_buff_len = body_length;
    }
    
    /* Send header */
    err = httpd_send(req, req->resp_headerbuff, req->resp_headerbuff_len);
    if (err) {
        return err;
    }
    return HTTPD_OK;
}


ICACHE_FLASH_ATTR
err_t httpd_response(struct httpd_request *req, char *status, 
        char *contenttype, char *content, uint32_t contentlength, 
        char **headers, uint8_t headers_count) {
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
    struct httpd_request *r = _findrequest(conn);
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
    struct httpd_request *r = _findrequest(conn);
    if (r != NULL) {
        _deleterequest(r, true);
    }
}


static ICACHE_FLASH_ATTR
void _client_connected(void *arg) {
    struct espconn *conn = arg;
    _ensurerequest(conn, (struct httpd_request**) &conn->reverse);
    os_printf(
            "Connected: "IPPORT_FMT".\r\n", 
            remoteinfo((struct httpd_request*) conn->reverse)
            );
    espconn_regist_recvcb(conn, _client_recv);
    espconn_regist_disconcb(conn, _client_disconnected);
    espconn_regist_sentcb(conn, _send_response_body);
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
    s->requests = os_zalloc(sizeof(struct httpd_request*) * HTTPD_MAXCONN);

    conn->type = ESPCONN_TCP;
    conn->state = ESPCONN_NONE;
    conn->proto.tcp = &s->esptcp;
    conn->proto.tcp->local_port = HTTPD_PORT;

    espconn_regist_connectcb(conn, _client_connected);
    espconn_regist_reconcb(conn, _client_recon);
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
        "HTTPD is listening on: "IPPORT_FMT", Max conn: %d\r\n", 
        localinfo(&s->esptcp),
        HTTPD_MAXCONN
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
    struct httpd_request **r = server->requests;

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

