#ifndef HTTPD_H_
#define HTTPD_H_

#include "session.h"
#include "router.h"
#include "response.h"
#include "request.h"
#include "multipart.h"


#include <ip_addr.h>
#include <espconn.h>


httpd_err_t httpd_init();
void httpd_deinit();
void httpd_recv(struct httpd_session *s);
#endif
