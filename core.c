#include "config.h"
#include "common.h"
#include "session.h"
#include "taskq.h"
#include "router.h"
#include "httpd.h"

#include <osapi.h>
#include <mem.h>


static struct espconn _conn;

// TODO: req headers
// TODO: resp headers



ICACHE_FLASH_ATTR 
void httpd_recv(struct httpd_session *s) {
    struct httpd_route *route;
    err_t err;
    
    /* Try to retrieve the currently processing request, if any. */
    if (s->handler == NULL) {
        /* Try to read http header. */
        err = http_request_parse(s);
        if (err == HTTPD_MORE) {
            /* Ignore and wait for more data */
            return;
        }
        if (err) {
            /* 400 Bad Request */
            httpd_response_badrequest(s);
            return;
        } 

        /* Find handler if this is the first packet. */
        route = router_find(s);
        if (route == NULL) {
            httpd_response_notfound(s);
            return;
        }
        s->handler = route->handler;
    }
    

    /* Pass the request to it's handler. */
    err = ((httpd_handler_t)s->handler)(s);
    if (err) {
        httpd_response_internalservererror(s);
        return;
    }
}


ICACHE_FLASH_ATTR 
err_t httpd_init(struct httpd_route **routes) {
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
    // TODO: Do not call me from any espconn callback
    session_deinit();
    router_deinit();
    os_delay_us(1000);
    taskq_push(HTTPD_SIG_SELFDESTROY, &_conn);
}

