#ifndef HTTPD_H_
#define HTTPD_H_

#include "session.h"
#include "router.h"

#include <ip_addr.h>
#include <espconn.h>



#define httpd_response_notok(s, status) \
    http_response(s, status, HTTPHEADER_CONTENTTYPE_TEXT, status, \
            strlen(status))

#define httpd_response_continue(s) \
    session_send(s, "HTTP/1.1 "HTTPSTATUS_CONTINUE"\r\n\r\n", 25);

#define httpd_response_notfound(s) \
    httpd_response_notok(s, HTTPSTATUS_NOTFOUND)

#define httpd_response_badrequest(s) \
    httpd_response_notok(s, HTTPSTATUS_BADREQUEST)

#define httpd_response_internalservererror(s) \
    httpd_response_notok(s, HTTPSTATUS_INTERNALSERVERERROR)

#define httpd_response_text(s, status, c, l) \
    http_response(s, status, HTTPHEADER_CONTENTTYPE_TEXT, (c), (l))


err_t httpd_init();
void httpd_deinit();
void httpd_recv(struct httpd_session *s);
#endif
