#include "common.h"
#include "taskq.h"
#include "config.h"

#include <mem.h>
#include <user_interface.h>


static os_event_t *_taskq;


static ICACHE_FLASH_ATTR 
void _worker(os_event_t *e) {
    switch (e->sig) {
        case HTTPD_SIG_RX:
            DEBUG("RX"CR);
            break;
        default:
            DEBUG("Invalid signal: %d"CR, e->sig);
        break;
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
