#include "response.h"
#include "session.h"

// TODO: Move it to config.h
#define HTTPD_STATIC_RESPHEADER_MAXLEN  1024
#define HTTPD_STATIC_RESPHEADER \
"HTTP/1.1 %s"CR \
"Server: esp8266-HTTPd/"__version__ CR \
"Content-Length: %d"CR \
"Connection: %s"CR 


ICACHE_FLASH_ATTR
void httpd_response_finalize(struct httpd_session *s, bool close) {
   struct httpd_request *r = &s->request;
    r->verb = NULL;
    r->path = NULL;
    r->query = NULL;
    r->contenttype = NULL;
    r->contentlen = 0;
    r->keepalive = false;
    r->remaining_contentlen = 0;
    r->handler = NULL;
    r->headerscount = 0;
    if (r->headers) {
        os_free(r->headers);
    }
    if (close) {
        session_close(s);
    }
    else {
        session_reset(s);
    }
}


ICACHE_FLASH_ATTR
httpd_err_t httpd_response_start(struct httpd_session *s, char *status, 
        struct httpd_header *headers, uint8_t headerscount, char *contenttype, 
        uint32_t contentlen, bool close) {
    httpd_err_t err;
    uint8_t i;
    size16_t tmplen;
    char tmp[HTTPD_STATIC_RESPHEADER_MAXLEN];
    tmplen = os_sprintf(tmp, HTTPD_STATIC_RESPHEADER, status, contentlen,
            close? "close": "keep-alive"); 
    
    err = session_resp_write(s, tmp, tmplen); 
    if (err) {
        return err;
    }
   
    /* Content type / length */
    if ((contenttype  != NULL) && (contentlen > 0)) {
        tmplen = os_sprintf(tmp, "Content-Type: %s"CR, contenttype);
        err = session_resp_write(s, tmp, tmplen); 
        if (err) {
            return err;
        }
    }

    DEBUG("demo index"CR);
    /* Write headers */
    for (i = 0; i < headerscount; i++) {
        tmplen = os_sprintf(tmp, "%s: %s"CR, headers[i].name, 
                headers[i].value);
        err = session_resp_write(s, tmp, tmplen); 
        if (err) {
            return err;
        }
    }
    /* Reset response write counter */
    s->resp_rb.writecounter = 0;
    
    /* Trigger taskq to send buffered data */
    err = session_send(s, CR, 2);
    if (err) {
        return err;
    }
    return HTTPD_OK;
}


ICACHE_FLASH_ATTR
httpd_err_t httpd_response(struct httpd_session *s, char *status,
        struct httpd_header *headers, uint8_t headerscount, char *contenttype, 
        char *content, uint32_t contentlen, bool close) {
    httpd_err_t err;
    bool forceclose = (close || (!s->request.keepalive));
    err = httpd_response_start(s, status, headers, headerscount, contenttype, 
            contentlen, forceclose);
    if (err) {
        return err;
    }
    
    err = session_send(s, content, contentlen);
    if (err) {
        return err;
    }

    httpd_response_finalize(s, forceclose);
    return HTTPD_OK;
}


