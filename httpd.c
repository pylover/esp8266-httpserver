#include "session.h"
#include "router.h"
#include "httpd.h"
#include "tcpd.h"


static struct espconn _conn;
static os_event_t *_taskq;


ICACHE_FLASH_ATTR 
void httpd_recv(struct httpd_session *s) {
    struct httpd_route *route;
    struct httpd_request *r = &s->request;
    httpd_err_t err;

    /* Try to retrieve the currently processing request, if any. */
    if (r->handler == NULL) {
        /* Try to read http header. */
        err = httpd_request_parse(s);
        if (err == HTTPD_MORE) {
            /* Ignore and wait for more data */
            return;
        }

        /* HTTP/1.1 100-continue */
        if (err == HTTPD_ERR_HTTPCONTINUE) {
            httpd_response_continue(s);
            return;
        }
        if (err) {
            /* 400 Bad Request */
            httpd_response_badrequest(s);
            return;
        } 
        
        /* Reset request write counter */
        s->req_rb.writecounter = RB_USED(&s->req_rb);

        /* Find and set handler if this is the first packet. */
        route = router_find(s);
        if (route == NULL) {
            httpd_response_notfound(s);
            return;
        }
        r->handler = route->handler;
    }
    
    /* Pass the request to it's handler. */
    err = ((httpd_handler_t)r->handler)(s);
    
    /* Consumer requested more data. */
    if (err == HTTPD_MORE) {
        return;
    }
    
    /* Error inside handler. */
    if (err) {
        DEBUG("Internal Server Error: %d", err);
        httpd_response_internalservererror(s);
        return;
    }
}



static ICACHE_FLASH_ATTR 
void _worker(os_event_t *e) {
    httpd_err_t err = ESPCONN_OK;

    switch (e->sig) {
        case HTTPD_SIG_RECV:
            httpd_recv((struct httpd_session *)e->par);
            break;
        case HTTPD_SIG_REJECT:
            err = tcpd_close((struct espconn*) e->par);
            break;
        case HTTPD_SIG_CLOSE:
            err = session_close((struct httpd_session *)e->par);
            break;
        case HTTPD_SIG_SEND:
            // TODO: encapsulate in new function
            err = session_resp_flush((struct httpd_session *)e->par);
            break;
        case HTTPD_SIG_SELFDESTROY:
            tcpd_deinit((struct espconn*) e->par);
            os_free(_taskq);
            break;
        default:
            DEBUG("Invalid signal: %d", e->sig);
            break;
    }
    if (err) {
        // TODO: Completely dispose request;
        // TODO: filter espconn errors
        tcpd_print_espconn_err(err);
    }
}


ICACHE_FLASH_ATTR 
httpd_err_t httpd_init(struct httpd_route *routes) {
    /* Init router */
    router_init(routes);
    /* Listen TCP */
    httpd_err_t err = tcpd_init(&_conn);
    if (err) {
        ERROR("Cannot listen: %d", err);
        return err;
    }
    INFO("HTTP Server is listening on: "IPPSTR".", 
            IPP2STR_LOCAL(_conn.proto.tcp));
    
    /* Initialize and allocate session based on HTTPD_MAXCONN. */
    err = session_init();
    if (err) {
        return err;
    }

    /* Setup OS task queue */
    _taskq = (os_event_t*)os_malloc(sizeof(os_event_t) * HTTPD_TASKQ_LEN);
    if (system_os_task(_worker, HTTPD_TASKQ_PRIO, _taskq, HTTPD_TASKQ_LEN)) {
        return HTTPD_OK;
    }
    return HTTPD_ERR_TASKQINIT;
}


ICACHE_FLASH_ATTR
void httpd_deinit() {
    session_deinit();
    router_deinit();
    os_delay_us(1000);
    HTTPD_SCHEDULE(HTTPD_SIG_SELFDESTROY, &_conn);
}

