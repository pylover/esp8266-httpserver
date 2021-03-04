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
    switch (e->sig) {
        case HTTPD_SIG_RECV:
            DEBUG("W: RECV"CR);
            struct httpd_session *s = (struct httpd_session *)e->par;
            char tmp[128];
            rb_size_t blen = session_req_len(s);
            rb_size_t len = session_read_req(s, tmp, blen);
            tmp[len] = 0;
            DEBUG("Sending data: %d,%d bytes: %s"CR, blen, len, tmp);
            err = espconn_send(s->conn, tmp, len);
            break;
        case HTTPD_SIG_REJECT:
            DEBUG("W: Reject"CR);
            struct espconn *conn = (struct espconn*) e->par;
            err = espconn_disconnect(conn);
            break;
        default:
            DEBUG("Invalid signal: %d"CR, e->sig);
            break;
    }
    if (err) {
        DEBUG("TASKQ: ERR: %d"CR, err);
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
