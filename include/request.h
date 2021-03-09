#ifndef REQUEST_H
#define REQUEST_H

#include "common.h"
#include "session.h"

#include <ip_addr.h>
#include <espconn.h>


#define HTTPD_REQUESTBODY_REMAINING(s) \
    MAX(0, ((int)(s)->request.contentlen) - ((int)(s)->req_rb.writecounter))

httpd_err_t http_request_parse(struct httpd_session *s);

#endif
