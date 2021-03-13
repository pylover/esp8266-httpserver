#ifndef HTTPD_COMMON_H
#define HTTPD_COMMON_H

#include "debug.h"
#include "config.h"
#include "datamodel.h"

#include <mem.h>
#include <osapi.h>
#include <ip_addr.h>
#include <espconn.h>
#include <c_types.h>


#define __version__     "2.0.0"


#define MIN(x, y) (((x) > (y))? (y): (x))
#define MAX(x, y) (((x) < (y))? (y): (x))


/* HTTPd errors */
#define HTTPD_OK                                  0
#define HTTPD_MORE                              -70
#define HTTPD_ERR_MAXCONNEXCEED                 -71
#define HTTPD_ERR_MEMFULL                       -72
#define HTTPD_ERR_TASKQINIT                     -73
#define HTTPD_ERR_TASKQ_FULL                    -74
#define HTTPD_ERR_BADSTARTLINE                  -75
#define HTTPD_ERR_BADHEADER                     -76
#define HTTPD_ERR_MAXHEADER                     -77
#define HTTPD_ERR_HTTPCONTINUE                  -78

#define HTTPD_MP_LASTCHUNK                      -80
#define HTTPD_ERR_MP_DONE                       -81
#define HTTPD_ERR_MP_ALREADYINITIALIZED         -82
#define HTTPD_ERR_MP_BADHEADER                  -83

/* HTTP Statuses */
#define HTTPSTATUS_CONTINUE             "100 Continue"
#define HTTPSTATUS_OK                   "200 OK"
#define HTTPSTATUS_BADREQUEST           "400 Bad Request"
#define HTTPSTATUS_NOTFOUND             "404 Not Found"
#define HTTPSTATUS_CONFLICT             "409 Conflict"
#define HTTPSTATUS_INTERNALSERVERERROR  "500 Internal Server Error"


/* HTTP Verbs */
#define HTTPVERB_ANY                  NULL


/* Content types */
#define HTTPHEADER_CONTENTTYPE_TEXT   "text/plain"
#define HTTPHEADER_CONTENTTYPE_HTML   "text/html"
#define HTTPHEADER_CONTENTTYPE_JPEG   "image/jpeg"
#define HTTPHEADER_CONTENTTYPE_ICON   "image/x-icon"
#define HTTPHEADER_CONTENTTYPE_BINARY "application/octet-stream"

/* Signals */
#define HTTPD_SIG_REJECT            1
#define HTTPD_SIG_CLOSE             2
#define HTTPD_SIG_SEND              3
#define HTTPD_SIG_SELFDESTROY       4
#define HTTPD_SIG_RECVUNHOLD        5

/* Flags */
#define HTTPD_FLAG_NONE        0x00
#define HTTPD_FLAG_CLOSE       0x01
#define HTTPD_FLAG_STREAM      0x02

/* Session statuses */
#define HTTPD_SESSIONSTATUS_IDLE        0
#define HTTPD_SESSIONSTATUS_RECVHOLD    1
#define HTTPD_SESSIONSTATUS_CLOSING     2
#define HTTPD_SESSIONSTATUS_CLOSED      3

/* Multipart Statuses */
#define HTTPD_MP_STATUS_BOUNDARY    0
#define HTTPD_MP_STATUS_HEADER      1
#define HTTPD_MP_STATUS_BODY        2


#define HTTPD_SCHEDULE(sig, arg) \
    system_os_post(HTTPD_TASKQ_PRIO, (sig), (arg))


#endif
