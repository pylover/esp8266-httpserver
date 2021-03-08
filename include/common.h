#ifndef _COMMON_H
#define _COMMON_H

#include "config.h"

#include <osapi.h>


#define __version__     "2.0.0"

#define CR  "\r\n"
#define IPPSTR  IPSTR":%d"
#define IPP2STR(t)  IP2STR((t)->remote_ip), (t)->remote_port
#define IPP2STR_LOCAL(t)  IP2STR((t)->local_ip), (t)->local_port


#define MIN(x, y) (((x) > (y))? (y): (x))

#if defined(GLOBAL_DEBUG_ON)

#define INFO( format, ... ) os_printf( format, ## __VA_ARGS__ )
#define DEBUG( format, ... ) os_printf( "%s:%d [%s] "format, \
        __FILE__, __LINE__, __func__, ## __VA_ARGS__ )
#define ERROR( format, ... ) os_printf( format, ## __VA_ARGS__ )

#else

#define INFO( format, ... )
#define DEBUG( format, ... )
#define ERROR( format, ... )

#endif


#define HTTPD_OK                    0
#define HTTPD_MORE                 -40
#define HTTPD_ERR_MAXCONNEXCEED    -50
#define HTTPD_ERR_MEMFULL          -51
#define HTTPD_ERR_TASKQINIT        -52
#define HTTPD_ERR_TASKQ_FULL       -53
#define HTTPD_ERR_BADSTARTLINE     -54
#define HTTPD_ERR_BADHEADER        -55
#define HTTPD_ERR_MAXHEADER        -55
#define HTTPD_ERR_HTTPCONTINUE     -56

/* HTTP Statuses */
#define HTTPSTATUS_CONTINUE             "100 Continue"
#define HTTPSTATUS_OK                   "200 Ok"
#define HTTPSTATUS_BADREQUEST           "400 Bad Request"
#define HTTPSTATUS_NOTFOUND             "404 Not Found"
#define HTTPSTATUS_INTERNALSERVERERROR  "500 Internal Server Error"

/* Content types */
#define HTTPHEADER_CONTENTTYPE_TEXT  "text/plain"
#define HTTPHEADER_CONTENTTYPE_HTML  "text/html"
#define HTTPHEADER_CONTENTTYPE_JPEG  "image/jpeg"
#define HTTPHEADER_CONTENTTYPE_ICON  "image/x-icon"
#define HTTPVERB_ANY                  NULL


#endif
