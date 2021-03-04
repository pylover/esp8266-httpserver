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
#define HTTPD_TIMEOUT    500
#endif

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

#endif
