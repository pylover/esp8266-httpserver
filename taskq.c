#include "common.h"
#include "session.h"
#include "taskq.h"
#include "ringbuffer.h"
#include "config.h"

#include <mem.h>
#include <user_interface.h>


static os_event_t *_taskq;


static ICACHE_FLASH_ATTR 
void _worker(os_event_t *e) {
    err_t err = ESPCONN_OK;
    struct espconn *conn;
    struct httpd_session *s;

    switch (e->sig) {
        case HTTPD_SIG_RECV:
            s = (struct httpd_session *)e->par;
            char tmp[128];
            rb_size_t blen = session_req_len(s);
            rb_size_t len = session_read_req(s, tmp, blen);
            tmp[len] = 0;
            err = espconn_send(s->conn, tmp, len);
            break;
        case HTTPD_SIG_REJECT:
            conn = (struct espconn*) e->par;
            err = espconn_disconnect(conn);
            break;
        case HTTPD_SIG_CLOSE:
            s = (struct httpd_session *)e->par;
            err = espconn_disconnect(s->conn);
            break;
        case HTTPD_SIG_SENT:
            break;
        case HTTPD_SIG_SELFDESTROY:
            conn = (struct espconn*) e->par;
            taskq_deinit();
            tcpd_deinit(conn);
            break;
        default:
            DEBUG("Invalid signal: %d"CR, e->sig);
            break;
    }
    if (err) {
        tcpd_print_err(err);
    }
}


ICACHE_FLASH_ATTR 
err_t taskq_init() {
    _taskq = (os_event_t*)os_malloc(sizeof(os_event_t) * HTTPD_TASKQ_LEN);
    if (system_os_task(_worker, HTTPD_TASKQ_PRIO, _taskq, HTTPD_TASKQ_LEN)) {
        return HTTPD_OK;
    }
    return HTTPD_ERR_TASKQINIT;
}


ICACHE_FLASH_ATTR 
void taskq_deinit() {
    os_free(_taskq);
}
