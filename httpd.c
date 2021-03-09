#include "config.h"
#include "common.h"
#include "session.h"
#include "taskq.h"
#include "router.h"
#include "httpd.h"

#include <osapi.h>
#include <mem.h>


static struct espconn _conn;

// TODO: resp headers


ICACHE_FLASH_ATTR 
void httpd_recv(struct httpd_session *s) {
    struct httpd_route *route;
    struct httpd_request *r = &s->request;
    err_t err;
    
    /* Try to retrieve the currently processing request, if any. */
    if (r->handler == NULL) {
        /* Try to read http header. */
        err = httpd_request_parse(s);
        if (err == HTTPD_MORE) {
            /* Ignore and wait for more data */
            return;
        }
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

        /* Find handler if this is the first packet. */
        route = router_find(s);
        if (route == NULL) {
            httpd_response_notfound(s);
            return;
        }
        r->handler = route->handler;
    }
    
    /* Pass the request to it's handler. */
    err = ((httpd_handler_t)r->handler)(s);
    if (err) {
        DEBUG("Internal Server Error: %d"CR, err);
        httpd_response_internalservererror(s);
        return;
    }
}


ICACHE_FLASH_ATTR 
err_t httpd_init(struct httpd_route *routes) {
    /* Init router */
    router_init(routes);
    /* Listen TCP */
    err_t err = tcpd_init(&_conn);
    if (err) {
        os_printf("Cannot listen: %d"CR, err);
        return err;
    }
    INFO("HTTP Server is listening on: "IPPSTR"."CR, 
            IPP2STR_LOCAL(_conn.proto.tcp));
    
    /* Initialize and allocate session based on HTTPD_MAXCONN. */
    err = session_init();
    if (err) {
        return err;
    }
    /* Setup os tasks */
    return taskq_init();
}


ICACHE_FLASH_ATTR
void httpd_deinit() {
    session_deinit();
    router_deinit();
    os_delay_us(1000);
    taskq_push(HTTPD_SIG_SELFDESTROY, &_conn);
}

