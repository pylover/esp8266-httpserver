#ifndef HTTPD_CONFIG_H
#define HTTPD_CONFIG_H

#include "user_config.h"


#define __httpdversion__  "2.0.0"


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
#define HTTPD_MAXCONN    4
#endif

/**
 * TCP timeout in seconds.
 */
#ifndef HTTPD_TIMEOUT
#define HTTPD_TIMEOUT    50
#endif

/**
 * Maximum request header size;
 */
#ifndef HTTPD_REQ_HEADERSIZE
#define HTTPD_REQ_HEADERSIZE    2048
#endif

/**
 * Request ringbuffer size.
 * Must be power of 2.
 */
#ifndef HTTPD_REQ_BUFFSIZE
#define HTTPD_REQ_BUFFSIZE      4096
#endif

/**
 * Response ringbuffer size.
 * Must be power of 2.
 */
#ifndef HTTPD_RESP_BUFFSIZE
#define HTTPD_RESP_BUFFSIZE     4096 
#endif

/**
 * Chunk size for IO
 */
#ifndef HTTPD_CHUNK
#define HTTPD_CHUNK     2920
#endif

/**
 * OS task queue depth.
 */
#ifndef HTTPD_TASKQ_LEN      
#define HTTPD_TASKQ_LEN      3
#endif

/**
 * OS task priority.
 * Three priorities are supported: 0/1/2; 0 is the lowest priority. 
 * This means only 3 tasks are allowed to be set up.
 */
#define HTTPD_TASKQ_PRIO     2

/**
 * Maximum request headers.
 */
#define HTTPD_REQ_HEADERS_MAX  16

/**
 * Maximum allowed querystring name.
 */
#define HTTPD_QS_NAME_MAX       32

/**
 * Maximum allowed querystring value.
 */
#define HTTPD_QS_VALUE_MAX      512 

/**
 * Request static header.
 */
#define HTTPD_STATIC_RESPHEADER_MAXLEN  1024
#define HTTPD_STATIC_RESPHEADER \
"HTTP/1.1 %s"CR \
"Server: esp8266-HTTPd/"__httpdversion__ CR \
"Connection: %s"CR 

/** 
 * Multipart callback cunk size
 */
#define HTTPD_MP_CHUNK      HTTPD_CHUNK

/**
 * Multipart field header limit.
 */
#define HTTPD_MP_HEADERSIZE     1024

#endif
