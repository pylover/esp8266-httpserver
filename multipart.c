#include "multipart.h"
#include "session.h"


static ICACHE_FLASH_ATTR 
void _multipart_finalize(struct httpd_multipart *m) {
    struct httpd_session *s = m->session;
    
    /* Restore previously backed-up handler. */
    s->request.handler = m->handlerbackup;
    
    /* Clear references. */
    s->reverse = NULL;
    os_free(m);
}


static ICACHE_FLASH_ATTR 
httpd_err_t _multipart_header_parse(struct httpd_multipart *m) {
    size16_t len; 
    size8_t boundarylen = m->boundarylen;
    httpd_err_t err;
    char *c = m->header_buff;  // Cursor
    char *e;  // Line end

    /* Dry read the boundary line */
    err = session_dryrecv_until(m->session, c, HTTPD_MP_HEADERSIZE, CR, 2, 
            &len);
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
        (os_strncmp(c + 2, m->boundary, m->boundarylen) != 0)) {
        return HTTPD_ERR_MP_BADHEADER;
    }
   
    /* Detect last boundary */
    if (os_strncmp(c + m->boundarylen + 2, "--", 2) == 0) {
        return HTTPD_ERR_MP_DONE;
    }

    /* Read entire header into buffer. */
    err = session_recv_until(m->session, c, HTTPD_MP_HEADERSIZE, CR CR, 4, 
            &len);
    if (err == RB_ERR_NOTFOUND) {
        return HTTPD_MORE;
    }

    /* Parsing multipart field header.
     * (https://tools.ietf.org/html/rfc7578#section-4)
     */
    
    /* Null terminate the boundary line */
    if ((e = os_strstr(c, CR)) == NULL) {
        return HTTPD_ERR_MP_BADHEADER;
    }
    e[0] = 0;

    /* Content-Disposition */
    c = e + 2;
    if ((e = os_strstr(c, CR)) == NULL) {
        return HTTPD_ERR_MP_BADHEADER;
    }
    e[0] = 0;

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
    c = e + 2;
    
    /* Filename */
    if ((c = os_strstr(c, "filename=\"")) != NULL) {
        c += 10;
        if ((e = os_strstr(c, "\"")) == NULL ) {
            return HTTPD_ERR_MP_BADHEADER;
        }
        e[0] = 0;
        m->filename = c;
        c = e + 2;
    }
    
    /* Content-Type header if presents. */
    if (os_strncmp(c, CR, 2) != 0) {
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
httpd_err_t _multipart_handler(struct httpd_session *s) {
    struct httpd_multipart *m = (struct httpd_multipart*) s->reverse;
    httpd_err_t err;
     
    if (m->field == NULL) {
        /* Parse field header */ 
        err = _multipart_header_parse(m);
        
        /* Done parsing if it was the last boundary. */
        if (err == HTTPD_ERR_MP_DONE) {
            _multipart_finalize(m);
            return HTTPD_OK;
        }
        if (err) {
            return err;
        }
        
        /* Reset ringbuffer to carry the field's content. */
        RB_RESET(&m->rb);
        DEBUG("MP Field: %s %s %s"CR, m->field, m->filename, m->contenttype);
    }
    
    
    /* TODO: Process content */ 
    return HTTPD_OK;
}


ICACHE_FLASH_ATTR 
httpd_err_t httpd_form_multipart_parse(struct httpd_session *s, 
        httpd_multipart_handler_t handler) {
    struct httpd_multipart *m;
    
    /* Prevent dubble initialization per session. */
    if (s->reverse == NULL) {
        return HTTPD_ERR_MP_ALREADYINITIALIZED;
    }

    /* Initialize Multipart */
    m = os_zalloc(sizeof(struct httpd_multipart));
    rb_init(&m->rb, m->buff, HTTPD_MP_BUFFSIZE, RB_OVERFLOW_ERROR);
    
    /* Boundary */
    m->boundary = os_strstr(s->request.contenttype, "boundary");
    if (m->boundary == NULL) {
        return HTTPD_ERR_MP_NOBOUNDARY;
    }
    m->boundary += 9;
    m->boundarylen = os_strlen(m->boundary);

    /* Backup handler and set own instead */
    m->handlerbackup = s->request.handler;
    s->request.handler = _multipart_handler;
    
    /* Update references */
    m->session = s;
    s->reverse = m;
    
    /* Trigger callback mannualy, because sometimes it's the last chance to
     * parse the form. */
    return _multipart_handler(s);
}
