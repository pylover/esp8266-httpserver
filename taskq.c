#include "common.h"
#include "session.h"
#include "taskq.h"
#include "ringbuffer.h"
#include "config.h"

#include <mem.h>
#include <user_interface.h>


static os_event_t *_taskq;

#define HTTPD_CHUNK     1400


static ICACHE_FLASH_ATTR 
void _worker(os_event_t *e) {
    char tmp[HTTPD_CHUNK];
    err_t err = ESPCONN_OK;
    struct espconn *conn;
    struct httpd_session *s;
    rb_size_t len;

    switch (e->sig) {
        case HTTPD_SIG_RECV:
            err = httpd_recv((struct httpd_session *)e->par);
            break;
        case HTTPD_SIG_REJECT:
            conn = (struct espconn*) e->par;
            err = espconn_disconnect(conn);
            break;
        case HTTPD_SIG_CLOSE:
            s = (struct httpd_session *)e->par;
            err = espconn_disconnect(s->conn);
            break;
        case HTTPD_SIG_SEND:
            s = (struct httpd_session *)e->par;
            len = session_resp_read(s, tmp, HTTPD_CHUNK);
            if (len <= 0) {
                break;
            }
            err = espconn_send(s->conn, tmp, len);
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
