#include "config.h"
#include "common.h"
#include "session.h"
#include "taskq.h"
#include "router.h"
#include "httpd.h"

#include <osapi.h>
#include <mem.h>


static struct espconn _conn;


ICACHE_FLASH_ATTR 
err_t httpd_request_parse(struct httpd_session *s) {
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

    DEBUG("Path"CR);
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
    
    DEBUG("%s %s"CR, r->verb, r->path);
    /* Parse headers */

    return HTTPD_OK;
}


ICACHE_FLASH_ATTR 
err_t httpd_recv(struct httpd_session *s) {
    struct httpd_request *req;
    err_t err;
    
    /* Try to retrieve the currently processing request, if any. */
    req = &s->request;
    if (req->verb == NULL) {
        /* Try to read http header. */
        err = httpd_request_parse(s);
        if (err == HTTPD_MORE) {
            /* Ignore and wait for more data */
            return HTTPD_OK;
        }
        if (err) {
            /* 400 Bad Request */
            return err;
        } 
    }
    
    ///* Find handler if this is the first packet. */
    //struct httpd_route *route = router_find(s);
    //if (route == NULL) {
    //    // TODO: return 404
    //    /* 404 */
    //}
    //s->handler = route->handler;

    ///* Pass the request to it's handler. */
    //err = ((httpd_handler_t)s->handler)(s);
    //if (err) {
    //    
    //    // TODO: 500
    //    /* 500 Internal server error. */
    //}
    
    return HTTPD_OK;
}


ICACHE_FLASH_ATTR 
err_t httpd_init() {
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
    os_delay_us(1000);
    taskq_push(HTTPD_SIG_SELFDESTROY, &_conn);
}

