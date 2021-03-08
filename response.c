#include "response.h"
#include "session.h"

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
Date: Sat, 06 Mar 2021 01:41:06 GMT
TODO: connectino keep-alive/close
Connection: close
*/


ICACHE_FLASH_ATTR
void httpd_response_finalize(struct httpd_session *s) {
   struct httpd_request *r = &s->request;
    r->verb = NULL;
    r->path = NULL;
    r->query = NULL;
    r->contenttype = NULL;
    r->contentlen = 0;
    r->remaining_contentlen = 0;
    r->handler = NULL;
    r->headerscount = 0;
    if (r->headers) {
        os_free(r->headers);
    }
    session_reset(s);
}


ICACHE_FLASH_ATTR
err_t httpd_response_start(struct httpd_session *s, char *status, 
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
err_t httpd_response(struct httpd_session *s, char *status,
        char *contenttype, char *content, uint32_t contentlen) {
    err_t err;
    
    err = httpd_response_start(s, status, contenttype, contentlen);
    if (err) {
        return err;
    }
    
    err = session_send(s, content, contentlen);
    if (err) {
        return err;
    }

    httpd_response_finalize(s);
    return HTTPD_OK;
}


