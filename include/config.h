#ifndef CONFIG_H
#define CONFIG_H

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
#define HTTPD_TIMEOUT    50
#endif

/**
 * Maximum request header size;
 */
#define HTTPD_REQ_HEADERSIZE    2048

/**
 * Request ringbuffer size.
 * Must be power of 2.
 */
#define HTTPD_REQ_BUFFSIZE      2048 

/**
 * Response ringbuffer size.
 * Must be power of 2.
 */
#define HTTPD_RESP_BUFFSIZE     2048

/**
 * Chunk size for IO
 */
#define HTTPD_CHUNK     1500

/**
 * OS task queue depth.
 */
#define HTTPD_TASKQ_LEN      4

/**
 * OS task priority.
 * Three priorities are supported: 0/1/2; 0 is the lowest priority. 
 * This means only 3 tasks are allowed to be set up.
 */
#define HTTPD_TASKQ_PRIO     2

/**
 * Maximum request headers.
 */
#define HTTPD_REQ_HEADERS_MAX  10

/**
 * Maximum allowed querystring name.
 */
#define HTTPD_QS_NAME_MAX       32

/**
 * Maximum allowed querystring value.
 */
#define HTTPD_QS_VALUE_MAX      512 

/**
 * Multipart ringbuffer size.
 * Must be power of 2.
 */
//#define HTTPD_MP_BUFFSIZE       HTTPD_REQ_BUFFSIZE 
#define HTTPD_MP_BUFFSIZE       8192 

/** 
 * Multipart callback cunk size
 */
#define HTTPD_MP_CHUNKSIZE      HTTPD_CHUNK

/**
 * Multipart field header limit.
 */
#define HTTPD_MP_HEADERSIZE     1024

#endif
