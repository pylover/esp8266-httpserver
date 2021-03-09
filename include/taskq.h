#ifndef TASKQ_H
#define TASKq_H


#include <ip_addr.h>
#include <espconn.h>


#define HTTPD_SIG_REJECT            1
#define HTTPD_SIG_RECV              2
#define HTTPD_SIG_CLOSE             3
#define HTTPD_SIG_SEND              4
#define HTTPD_SIG_SELFDESTROY       5


#define taskq_push(sig, arg) system_os_post(HTTPD_TASKQ_PRIO, (sig), (arg))


httpd_err_t taskq_init();
void taskq_deinit();
#endif
