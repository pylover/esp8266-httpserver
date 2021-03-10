#include "multipart.h"


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
httpd_err_t _parse_header(struct httpd_multipart *m) {

}


static ICACHE_FLASH_ATTR 
httpd_err_t _multipart_handler(struct httpd_session *s) {
    struct httpd_multipart *m = (struct httpd_multipart*) s->reverse;
    httpd_err_t err;
     
    if (m->field == NULL) {
        /* Parse field header */ 
        err = _parse_header(m);
        if (err) {
            return err;
        }
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
        return HTTPD_MULTIPART_ALREADY_INITIALIZED;
    }

    /* Initialize Multipart */
    m = os_zalloc(sizeof(struct httpd_multipart));
    rb_init(&m->rb, m->buff, HTTPD_MP_BUFFSIZE, RB_OVERFLOW_ERROR);
    
    /* TODO: Boundary */
    
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
