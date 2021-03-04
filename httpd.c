#include "config.h"
#include "httpd.h"
#include "common.h"
#include "session.h"

#include <osapi.h>
#include <mem.h>


static struct espconn _conn;


ICACHE_FLASH_ATTR 
err_t httpd_init() {
    /* Listen TCP */
    err_t err = tcpd_init(&_conn);
    if (err) {
        os_printf("Cannot listen: %d"CR, err);
        return err;
    }
    os_printf("HTTP Server is listening on: "IPPSTR"."CR,  
            IPP2STR_LOCAL(_conn.proto.tcp));
    
    /* Initialize and allocate session based on HTTPD_MAXCONN. */
    session_init();

    /* Setup os tasks */
    return taskq_init();
}


ICACHE_FLASH_ATTR
void httpd_deinit() {
    // TODO: Do not call me from any espconn callback
    taskq_deinit();
    session_deinit();
    tcpd_deinit(&_conn);
}

