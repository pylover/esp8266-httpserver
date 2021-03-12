#ifndef response_h
#define response_h

#include "datamodel.h"


#define httpd_response_notok(s, status) \
    httpd_response(s, status, NULL, 0, \
            HTTPHEADER_CONTENTTYPE_TEXT, status CR, \
            strlen(status) + 2, HTTPD_FLAG_CLOSE)

#define httpd_response_continue(s) \
    session_send(s, "HTTP/1.1 "HTTPSTATUS_CONTINUE"\r\n\r\n", 25);

#define httpd_response_notfound(s) \
    httpd_response_notok(s, HTTPSTATUS_NOTFOUND)

#define httpd_response_badrequest(s) \
    httpd_response_notok(s, HTTPSTATUS_BADREQUEST)

#define httpd_response_conflict(s) \
    httpd_response_notok(s, HTTPSTATUS_CONFLICT)

#define httpd_response_internalservererror(s) \
    httpd_response_notok(s, HTTPSTATUS_INTERNALSERVERERROR)

#define httpd_response_text(s, status, c, l) \
    httpd_response(s, status, NULL, 0, HTTPHEADER_CONTENTTYPE_TEXT, (c), \
            (l), HTTPD_FLAG_NONE)


#endif
