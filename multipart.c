#include "multipart.h"
#include "session.h"
#include "response.h"


static ICACHE_FLASH_ATTR 
httpd_err_t _multipart_finalize(struct httpd_multipart *m) {
    struct httpd_session *s = m->session;
    httpd_err_t err;
   
    /* Run the handler for the last time. */
    err = ((httpd_multipart_handler_t)m->handler)(m, NULL, 0, true, true);
    if (err) {
        return err;
    }

    /* Restore previously backed-up handler. */
    s->request.handler = m->handlerbackup;
    
    /* Clear references. */
    s->reverse = NULL;
    os_free(m);
    return HTTPD_OK;
}


static ICACHE_FLASH_ATTR 
httpd_err_t _multipart_boundary_parse(struct httpd_multipart *m) {
    size16_t len; 
    size8_t boundarylen = m->boundarylen;
    httpd_err_t err;
    char *c = m->header_buff;  // Cursor
    char *e;  // Line end

    /* Read entire line into buffer. */
    err = session_recv_until(m->session, c, HTTPD_MP_HEADERSIZE, CR, 2, &len);
    if (err == RB_ERR_NOTFOUND) {
        return HTTPD_MORE;
    }
    
    /* Null terminate the boundary line */
    if ((e = os_strstr(c, CR)) == NULL) {
        return HTTPD_ERR_MP_BADHEADER;
    }
    e[0] = 0;
    
    /* Ensure boundary */
    if ((os_strncmp(c, "--", 2) != 0) || 
        (os_strncmp(c + 2, m->boundary, boundarylen) != 0)) {
        return HTTPD_ERR_MP_BADHEADER;
    }
   
    /* Detect last boundary */
    if (os_strncmp(c + boundarylen + 2, "--", 2) == 0) {
        return HTTPD_ERR_MP_DONE;
    }
    return HTTPD_OK;
}


static ICACHE_FLASH_ATTR 
httpd_err_t _multipart_header_parse(struct httpd_multipart *m) {
    size16_t len; 
    httpd_err_t err;
    char *c = m->header_buff;  // Cursor
    char *e;  // Line end

    /* Read entire header into buffer. */
    err = session_recv_until(m->session, c, HTTPD_MP_HEADERSIZE, CR CR, 4, 
            &len);
    if (err == RB_ERR_NOTFOUND) {
        return HTTPD_MORE;
    }

    /* Null terminate the Content-Disposition line */
    if ((e = os_strstr(c, CR)) == NULL) {
        return HTTPD_ERR_MP_BADHEADER;
    }
    e[0] = 0;

    /* Content-Disposition */
    if ((c = os_strstr(c, "Content-Disposition: ")) == NULL) {
        return HTTPD_ERR_MP_BADHEADER;
    }
    c += 30;

    /* Field's name */
    if ((c = os_strstr(c, "name=\"")) == NULL) {
        return HTTPD_ERR_MP_BADHEADER;
    }
    c += 6;
    if ((e = os_strstr(c, "\"")) == NULL ) {
        return HTTPD_ERR_MP_BADHEADER;
    }
    e[0] = 0;
    m->field = c;
    c = e + 3;
    
    /* Filename */
    if ((c = os_strstr(c, "filename=\"")) != NULL) {
        c += 10;
        if ((e = os_strstr(c, "\"")) == NULL ) {
            return HTTPD_ERR_MP_BADHEADER;
        }
        e[0] = 0;
        m->filename = c;
        c = e + 3;
    }
    else {
        /* Reset c (cursor) to prevent NULL pointer fatal. */
        c = e + 3;
    }
    
    /* Content-Type header if presents. */
    if (os_strncmp(c, CR, 2) != 0) {  // line[:2] != CR
        if ((e = os_strstr(c, CR)) == NULL) {
            return HTTPD_ERR_MP_BADHEADER;
        }
        e[0] = 0;
    
        if ((c = os_strstr(c, "Content-Type: ")) == NULL) {
            return HTTPD_ERR_MP_BADHEADER;
        }
        m->contenttype = c + 14;
    }
    return HTTPD_OK; 
}


static ICACHE_FLASH_ATTR 
httpd_err_t _multipart_body_parse(struct httpd_multipart *m) {
    httpd_err_t err;
    char tmp[HTTPD_MP_CHUNK + 1];
    size16_t len;
    int fieldlen;
    bool lastchunk;
    char *nextfield;
    
    /* Read from buffer, and reamins read needle unchanged. */
    len = session_dryrecv(m->session, tmp, HTTPD_MP_CHUNK);
     
    /* Searching for the next boundary */
    nextfield = memmem(tmp, len, m->boundary, m->boundarylen);
    if (nextfield != NULL) {
        /* End of field found */
        nextfield -= 4;
        fieldlen = nextfield - tmp;
        lastchunk = true;
    }
    else {
        /* End of field not found */
        lastchunk = false;
        fieldlen = len;

        /* Check for suspicious trailing */
        char *trailing = tmp + (len - m->boundarylen);
        char *matchp = memmem(trailing, m->boundarylen, m->boundary, 1);
        if (matchp != NULL) {
            int tail = m->boundarylen - (matchp - trailing);
            matchp = memmem(matchp, tail, m->boundary, tail);
            if (matchp != NULL) {
                /* boundary partialy matched */
                fieldlen -= tail;
            }
        }
        
    }
    
    if (!lastchunk) {
        if (fieldlen <= 4) {
            return HTTPD_MORE;
        }
        else {
            fieldlen -= 4;
        }
    }
    
    /* Call handler */
    err = ((httpd_multipart_handler_t)m->handler)(m, tmp, fieldlen, lastchunk, 
            false);
    if (err) {
        return err;
    }

    if (lastchunk) {
        fieldlen += 2;
    }
    session_recv_skip(m->session, fieldlen);
    
    if (lastchunk) {
        return HTTPD_MP_LASTCHUNK;
    }
    return HTTPD_OK;
}


static ICACHE_FLASH_ATTR 
httpd_err_t _multipart_handler(struct httpd_session *s) {
    struct httpd_multipart *m = (struct httpd_multipart*) s->reverse;
    httpd_err_t err;
   
    while (session_req_len(s) > (m->boundarylen + 2)) {
        /* buff len */
        switch (m->status) {
            case HTTPD_MP_STATUS_BOUNDARY:
                /* Boundary line */
                err = _multipart_boundary_parse(m);
                if (err == HTTPD_ERR_MP_DONE) {
                    /* Done parsing if it was the last boundary. */
                    return _multipart_finalize(m);
                }
                if (err == HTTPD_MORE) {
                    /* More data needed for boundary */
                    return err;
                }
                if (err) {
                    return err;
                }
                m->status = HTTPD_MP_STATUS_HEADER;
                break;

            case HTTPD_MP_STATUS_HEADER:
                /* Parse field header  */
                err = _multipart_header_parse(m);
                if (err == HTTPD_MORE) {
                    /* More data needed for header */
                    return err;
                }
                if (err) {
                    return err;
                }
                m->status = HTTPD_MP_STATUS_BODY;
                break;

            case HTTPD_MP_STATUS_BODY:
                /* Field Body */
                err = _multipart_body_parse(m);
                if (err == HTTPD_MP_LASTCHUNK) {
                    m->status = HTTPD_MP_STATUS_BOUNDARY;
                    break;
                }
                if (err == HTTPD_MORE) {
                    /* More data needed for body */
                    return err;
                }
                if (err) {
                    return err;
                }
                break;
        }
    }
    return HTTPD_MORE;
}


ICACHE_FLASH_ATTR 
httpd_err_t httpd_form_multipart_parse(struct httpd_session *s, 
        httpd_multipart_handler_t handler) {
    struct httpd_multipart *m;
    struct httpd_request *r = &s->request; 
    /* Prevent dubble initialization per session. */
    if (s->reverse != NULL) {
        return HTTPD_ERR_MP_ALREADYINITIALIZED;
    }

    /* Initialize Multipart */
    m = os_zalloc(sizeof(struct httpd_multipart));
    m->status = HTTPD_MP_STATUS_BOUNDARY;
    m->handler = handler;
    
    /* Boundary */
    if (r->contenttype == NULL) {
        return httpd_response_badrequest(s);
    }
    m->boundary = os_strstr(r->contenttype, "boundary");
    if (m->boundary == NULL) {
        return httpd_response_badrequest(s);
    }
    m->boundary += 9;
    m->boundarylen = os_strlen(m->boundary);

    /* Backup handler and set own instead */
    m->handlerbackup = r->handler;
    r->handler = _multipart_handler;
    
    /* Update references */
    m->session = s;
    s->reverse = m;
    
    /* Trigger callback mannualy, because sometimes it's the last chance to
     * parse the form. */
    return _multipart_handler(s);
}
