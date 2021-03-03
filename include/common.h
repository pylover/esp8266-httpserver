#ifndef _COMMON_H
#define _COMMON_H

#define IPPSTR  IPSTR":%d"
#define IPP2STR(t)  IP2STR((t)->remote_ip), (t)->remote_port
#define IPP2STR_LOCAL(t)  IP2STR((t)->local_ip), (t)->local_port


#endif

