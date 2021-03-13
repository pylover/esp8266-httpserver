#include "response.h"
#include "session.h"


ICACHE_FLASH_ATTR
httpd_err_t httpd_send(struct httpd_session *s, char * data, size16_t len) {
    httpd_err_t err;
    char tmp[HTTPD_CHUNK];
    size16_t tmplen;
   
    /* Write buffer if any data */
    if ((data != NULL) && (len > 0)) {
        httpd_err_t err = rb_write(&s->resp_rb, data, len);
        if (err) {
            return err;
        }
    }

    tmplen = HTTPD_RESP_DRYREAD(s, tmp, HTTPD_CHUNK);
    /* Reading data from response buffer to send: %d */
    if (tmplen <= 0) {
        if (s->status == HTTPD_SESSIONSTATUS_CLOSING) {
            err = TCPD_CLOSE(s->conn);
            if (err) {
                return err;
            }
            s->status = HTTPD_SESSIONSTATUS_CLOSED;
        }
        return HTTPD_OK;
    }
    
    /* espconn_send: %d */
    err = espconn_send(s->conn, tmp, tmplen);
    if (err == ESPCONN_MAXNUM) {
        /* send buffer is full. wait for espconn sent callback. */
        return HTTPD_OK;
    }
    else if (err) {
        return err;
    }
    
    /* Skip %d bytes */
    HTTPD_RESP_SKIP(s, tmplen);
    return HTTPD_OK;
}



ICACHE_FLASH_ATTR
void httpd_response_finalize(struct httpd_session *s, httpd_flag_t flags) {
    struct httpd_request *r = &s->request;
    /* Cleanup & Zero the request */
    if (r->headers) {
        os_free(r->headers);
    }
    memset(r, 0, sizeof(struct httpd_request));

    /* Schedule to close connection when all response is sent. */
    bool close = flags & HTTPD_FLAG_CLOSE;
    if (close) {
        s->status = HTTPD_SESSIONSTATUS_CLOSING;
    }

    SESSION_RESET(s);
}


ICACHE_FLASH_ATTR
httpd_err_t httpd_response_start(struct httpd_session *s, char *status, 
        struct httpd_header *headers, uint8_t headerscount, char *contenttype, 
        uint32_t contentlen, httpd_flag_t flags) {
    httpd_err_t err;
    uint8_t i;
    size16_t tmplen;
    char tmp[HTTPD_STATIC_RESPHEADER_MAXLEN];
    bool close = flags & HTTPD_FLAG_CLOSE;

    tmplen = os_sprintf(tmp, HTTPD_STATIC_RESPHEADER, status,
            close? "close": "keep-alive"); 
    
    err = HTTPD_RESP_WRITE(s, tmp, tmplen); 
    if (err) {
        return err;
    }
    
    /* Content length */
    if (!(flags & HTTPD_FLAG_STREAM)) {
        tmplen = os_sprintf(tmp, "Content-Length: %d"CR, contentlen);
        err = HTTPD_RESP_WRITE(s, tmp, tmplen); 
        if (err) {
            return err;
        }
    }

    /* Content type */
    if ((contenttype  != NULL) && 
            ((contentlen > 0) || (flags & HTTPD_FLAG_STREAM))) {
        tmplen = os_sprintf(tmp, "Content-Type: %s"CR, contenttype);
        err = HTTPD_RESP_WRITE(s, tmp, tmplen); 
        if (err) {
            return err;
        }
    }

    /* Write headers */
    for (i = 0; i < headerscount; i++) {
        tmplen = os_sprintf(tmp, "%s: %s"CR, headers[i].name, 
                headers[i].value);
        err = HTTPD_RESP_WRITE(s, tmp, tmplen); 
        if (err) {
            return err;
        }
    }
    /* Reset response write counter */
    s->resp_rb.writecounter = 0;
    
    /* Trigger taskq to send buffered data */
    err = httpd_send(s, CR, 2);
    if (err) {
        return err;
    }
    return HTTPD_OK;
}


ICACHE_FLASH_ATTR
httpd_err_t httpd_response(struct httpd_session *s, char *status,
        struct httpd_header *headers, uint8_t headerscount, char *contenttype, 
        char *content, uint32_t contentlen, httpd_flag_t flags) {
    httpd_err_t err;
    
    if (!s->request.keepalive) {
        flags |= HTTPD_FLAG_CLOSE;
    }

    err = httpd_response_start(s, status, headers, headerscount, contenttype, 
            contentlen, flags);
    if (err) {
        return err;
    }
    
    err = httpd_send(s, content, contentlen);
    if (err) {
        return err;
    }

    httpd_response_finalize(s, flags);
    return HTTPD_OK;
}


