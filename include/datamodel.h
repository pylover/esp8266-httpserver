#ifndef datamodel_h
#define datamodel_h

#include "common.h"
#include "ringbuffer.h"


struct httpd_header {
    char *name;
    char *value;
};


struct httpd_request {
    char header_buff[HTTPD_REQ_HEADERSIZE];
    char *verb;
    char *path;
    char *query;
    char *contenttype;
    bool keepalive;
    void *handler;
    uint32_t contentlen;
    uint32_t remaining_contentlen;
    struct httpd_header *headers;
    uint8_t headerscount;
};


/**
 * Represents connected client.
 */
struct httpd_session {
    uint8_t id;
    httpd_sessionstatus_t status;

    struct espconn *conn;
    void *reverse;

    uint8_t remote_ip[4];
    uint16_t remote_port;

    char req_buff[HTTPD_REQ_BUFFSIZE];
    struct ringbuffer req_rb;

    char resp_buff[HTTPD_RESP_BUFFSIZE];
    struct ringbuffer resp_rb;
    
    struct httpd_request request;
};


#define HTTPD_MP_STATUS_BOUNDARY    0
#define HTTPD_MP_STATUS_HEADER      1
#define HTTPD_MP_STATUS_BODY        2


struct httpd_multipart {
    struct httpd_session *session;

    char *boundary;
    size8_t boundarylen;

    void *handlerbackup;
    void *handler;

    uint8_t status;
    char header_buff[HTTPD_MP_HEADERSIZE];
    char *field;
    char *filename;
    char *contenttype;

    char buff[HTTPD_MP_BUFFSIZE];
    struct ringbuffer rb;
};


typedef httpd_err_t (*httpd_handler_t)(struct httpd_session *s);
typedef httpd_err_t (*httpd_multipart_handler_t)(struct httpd_multipart *m, 
        bool lastchunk, bool finalize);


struct httpd_route {
    char *verb;
    char *pattern;
    httpd_handler_t handler;
};


#endif
