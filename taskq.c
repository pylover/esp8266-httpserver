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
    httpd_err_t err = ESPCONN_OK;
    struct httpd_session *s;

    switch (e->sig) {
        case HTTPD_SIG_RECV:
            httpd_recv((struct httpd_session *)e->par);
            break;
        case HTTPD_SIG_REJECT:
            err = tcpd_reject((struct espconn*) e->par);
            break;
        case HTTPD_SIG_CLOSE:
            err = tcpd_close((struct httpd_session *)e->par);
            break;
        case HTTPD_SIG_SEND:
            // TODO: encapsulate in new function
            err = session_resp_flush((struct httpd_session *)e->par);
            break;
        case HTTPD_SIG_SELFDESTROY:
            taskq_deinit();
            tcpd_deinit((struct espconn*) e->par);
            break;
        default:
            DEBUG("Invalid signal: %d"CR, e->sig);
            break;
    }
    if (err) {
        // TODO: Completely dispose request;
        tcpd_print_err(err);
    }
}


ICACHE_FLASH_ATTR 
httpd_err_t taskq_init() {
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
