#ifndef TASKQ_H
#define TASKq_H


#include <ip_addr.h>
#include <espconn.h>


#define HTTPD_SIG_RX        1
#define HTTPD_SIG_CLOSE     2


#define taskq_push(sig, arg) system_os_post(HTTPD_TASKQ_PRIO, (sig), (arg))


err_t taskq_init();
void taskq_deinit();
#endif
