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
#define HTTPD_REQ_HEADERSIZE      2048

/**
 * Request ringbuffer size.
 * Must be power of 2.
 */
#define HTTPD_REQ_BUFFSIZE      4096

/**
 * Response ringbuffer size.
 * Must be power of 2.
 */
#define HTTPD_RESP_BUFFSIZE     4096

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


/* Needed by ringbuffer. */
#define  FUNC_ATTR       ICACHE_FLASH_ATTR

/**
 * Initial malloc for req headers, if headers are more than thid value,
 * the array will be reallocated with bigger size.
 */
#define HTTPD_REQ_HEADERS_INITIAL_ALLOCATE  10


#endif
