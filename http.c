#include "session.h"
#include "http.h"

#include <mem.h>
#include <osapi.h>


// TODO: Move it to config.h
#define HTTPD_STATIC_RESPHEADER_MAXLEN  1024
#define HTTPD_STATIC_RESPHEADER \
"HTTP/1.1 %s"CR \
"Server: esp8266-httpserver/"__version__ CR \
"Content-Type: %s"CR \
"Content-Length: %d"CR 

/*
HTTP/1.1 400 Bad Request
Server: nginx/1.18.0 (Ubuntu)
Date: Sat, 06 Mar 2021 01:41:06 GMT
Content-Type: text/html
Content-Length: 166
TODO: connectino keep-alive/close
Connection: close
*/

ICACHE_FLASH_ATTR
void http_response_finalize(struct httpd_session *s) {
   struct httpd_request *r = &s->request;
    r->verb = NULL;
    r->path = NULL;
    r->query = NULL;
    r->contenttype = NULL;
    r->contentlen = 0;
    r->remaining_contentlen = 0;
    r->handler = NULL;
    if (r->headers) {
        os_free(r->headers);
    }
    session_reset(s);
}


ICACHE_FLASH_ATTR
err_t http_response_start(struct httpd_session *s, char *status, 
        char *contenttype, uint32_t contentlen) {
    err_t err;
    rb_size_t tmplen;
    char tmp[HTTPD_STATIC_RESPHEADER_MAXLEN];
    tmplen = os_sprintf(tmp, HTTPD_STATIC_RESPHEADER, status, contenttype, 
            contentlen); 
    
    err = session_resp_write(s, tmp, tmplen); 
    if (err) {
        return err;
    }
   
    // TODO:
    /* Write headers */

    err = session_send(s, CR, 2);
    if (err) {
        return err;
    }
    return HTTPD_OK;
}


ICACHE_FLASH_ATTR
err_t http_response(struct httpd_session *s, char *status,
        char *contenttype, char *content, uint32_t contentlen) {
    err_t err;
    
    err = http_response_start(s, status, contenttype, contentlen);
    if (err) {
        return err;
    }
    
    err = session_send(s, content, contentlen);
    if (err) {
        return err;
    }

    http_response_finalize(s);
    return HTTPD_OK;
}



ICACHE_FLASH_ATTR 
err_t http_request_parse(struct httpd_session *s) {
    rb_size_t len; 
    err_t err;
    struct httpd_request *r = &s->request;
    char *c = s->request.header_buff;
    
    /* Ignore primitive CRs */
    while (true) {
        /* Search for \r\n or just \n */
        err = session_recv_until(s, c, 2, "\n", 1, &len);
        if (err == RB_ERR_NOTFOUND) {
            break;
        }
    }
    
    /* Read entire header into buffer. */
    err = session_recv_until(s, c, HTTPD_REQ_HEADERSIZE, CR CR, 4, &len);
    if (err == RB_ERR_NOTFOUND) {
        return HTTPD_MORE;
    }

    /* Parsing start line.
     * (https://www.w3.org/Protocols/rfc2616/rfc2616-sec4.html)
     */
    
    /* Verb */
    r->verb = c;
    c = os_strchr(c, ' ');
    if (c == NULL) {
        return HTTPD_ERR_BADSTARTLINE;
    }
    c[0] = 0;

    /* Path */
    r->path = ++c;
    c = os_strchr(c, ' ');
    if (c == NULL) {
        c = os_strstr(r->path, CR);
        if (c == NULL) {
            return HTTPD_ERR_BADSTARTLINE;
        }
    }
    c[0] = 0;
    
    /* Query string */
    c = os_strchr(r->path, '?');
    if (c != NULL) {
        c[0] = 0;
        r->query = ++c;
    }
    
    /* Startline's CR */
    c = os_strstr(c, CR);
    if (c == NULL) {
        return HTTPD_ERR_BADSTARTLINE;
    }
    c += 2;
    
    //r->headers = (struct httpd_header **)os_zalloc(sizeof(struct httpd_header) 
    //        * HTTPD_REQ_HEADERS_INITIAL_ALLOCATE);
    //while (true) {
    //    c = os_strstr(
    //
    //}
    // TODO:
    /* If a request contains a message-body and a Content-Length is not given,
     * the server SHOULD respond with 400 (bad request) if it cannot determine
     * the length of the message, or with 411 (length required) if it wishes 
     * to insist on receiving a valid Content-Length.*/
    DEBUG("%s %s %s"CR, r->verb, r->path, r->query);
    /* Parse headers */

    return HTTPD_OK;
}


