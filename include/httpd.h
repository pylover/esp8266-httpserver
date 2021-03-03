#ifndef HTTPD_H_
#define HTTPD_H_

#include "connection.h"

#include <c_types.h>
#include <espconn.h>


/**
 * Listen port.
 *
 * default: 80
 */
#ifndef HTTPD_PORT
#define HTTPD_PORT    80
#endif

/**
 * Maximum concurrent connections.
 */
#ifndef HTTPD_MAXCONN
#define HTTPD_MAXCONN    2
#endif


/**
 * TCP timeout in seconds.
 */
#ifndef HTTPD_TIMEOUT
#define HTTPD_TIMEOUT    500
#endif




struct httpd {
    struct espconn connection;
    esp_tcp esptcp;
    
    struct httpd_conn **connections;
};


#endif
