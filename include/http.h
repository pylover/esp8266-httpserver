#ifndef HTTP_H
#define HTTP_H

#include "session.h"

#include <ip_addr.h>
#include <espconn.h>


err_t http_request_parse(struct httpd_session *s);

#endif
