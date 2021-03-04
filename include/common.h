#ifndef _COMMON_H
#define _COMMON_H

#include <osapi.h>


#define CR  "\r\n"
#define IPPSTR  IPSTR":%d"
#define IPP2STR(t)  IP2STR((t)->remote_ip), (t)->remote_port
#define IPP2STR_LOCAL(t)  IP2STR((t)->local_ip), (t)->local_port


#if defined(GLOBAL_DEBUG_ON)

#define INFO( format, ... ) os_printf( format, ## __VA_ARGS__ )
#define DEBUG( format, ... ) os_printf( format, ## __VA_ARGS__ )
#define ERROR( format, ... ) os_printf( format, ## __VA_ARGS__ )

#else

#define INFO( format, ... )
#define DEBUG( format, ... )
#define ERROR( format, ... )
#define FATAL( format, ... )

#endif


#define HTTPD_OK                    0
#define HTTPD_MAXCONNEXCEED        -4

#endif

