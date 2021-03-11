#ifndef _COMMON_H
#define _COMMON_H

#include "config.h"

#include <mem.h>
#include <osapi.h>
#include <c_types.h>
#include <ip_addr.h>
#include <espconn.h>


#define __version__     "2.0.0"

#define CR  "\r\n"
#define IPPSTR  IPSTR":%d"
#define IPP2STR(t)  IP2STR((t)->remote_ip), (t)->remote_port
#define IPP2STR_LOCAL(t)  IP2STR((t)->local_ip), (t)->local_port
#define STARTSWITH(str, searchfor) \
    (strncmp(searchfor, str, strlen(searchfor)) == 0)



#define MIN(x, y) (((x) > (y))? (y): (x))
#define MAX(x, y) (((x) < (y))? (y): (x))

#ifndef INFO
#if defined(GLOBAL_DEBUG_ON)

#define ANSI_RED       "\033[31m"
#define ANSI_GREEN     "\033[32m"
#define ANSI_YELLOW    "\033[33m"
#define ANSI_CLEAR     "\033[0m"

#define INFO( fmt, ... ) os_printf( fmt CR, ## __VA_ARGS__ )


#define ERROR( fmt, ... ) os_printf( \
        ANSI_RED"%s:%d"ANSI_CLEAR" [%s] "fmt CR, \
        __FILE__, __LINE__, __func__, ## __VA_ARGS__ )


#define DEBUG( fmt, ... ) os_printf( \
        ANSI_YELLOW"%s:%d"ANSI_CLEAR" [%s] "fmt CR, \
        __FILE__, __LINE__, __func__, ## __VA_ARGS__ )

#define CHK( fmt, ... ) os_printf( \
        ANSI_GREEN"%s:%d"ANSI_CLEAR" [%s] "fmt CR, \
        __FILE__, __LINE__, __func__, ## __VA_ARGS__ )
#else

#define INFO( format, ... )
#define DEBUG( format, ... )
#define ERROR( format, ... )
#define CHK( format, ... )

#endif
#endif


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
#define HTTPSTATUS_INTERNALSERVERERROR  "500 Internal Server Error"


/* HTTP Verbs */
#define HTTPVERB_ANY                  NULL


/* Content types */
#define HTTPHEADER_CONTENTTYPE_TEXT  "text/plain"
#define HTTPHEADER_CONTENTTYPE_HTML  "text/html"
#define HTTPHEADER_CONTENTTYPE_JPEG  "image/jpeg"
#define HTTPHEADER_CONTENTTYPE_ICON  "image/x-icon"

/* Signals */
#define HTTPD_SIG_REJECT            1
#define HTTPD_SIG_RECV              2
#define HTTPD_SIG_CLOSE             3
#define HTTPD_SIG_SEND              4
#define HTTPD_SIG_SELFDESTROY       5

/* Flags */
#define HTTPD_FLAG_NONE        0x00
#define HTTPD_FLAG_CLOSE       0x01
#define HTTPD_FLAG_STREAM      0x02

/* Session statuses */
#define HTTPD_SESSIONSTATUS_IDLE        0
#define HTTPD_SESSIONSTATUS_CLOSING     1
#define HTTPD_SESSIONSTATUS_CLOSED      2


#define HTTPD_SCHEDULE(sig, arg) \
    system_os_post(HTTPD_TASKQ_PRIO, (sig), (arg))


typedef uint8_t size8_t;
typedef uint16_t size16_t;
typedef uint32_t size32_t;
typedef sint8_t httpd_err_t;
typedef uint8_t httpd_flag_t;
typedef uint8_t httpd_sessionstatus_t;


#endif
