#ifndef REQUEST_H
#define REQUEST_H

typedef enum {
    HRS_IDLE = 0,
    HRS_REQ_HEADER,
    HRS_REQ_BODY,
    HRS_RESP_HEADER,
    HRS_RESP_BODY
} HttpRequestStatus;


typedef struct {
	int remote_port;
	uint8 remote_ip[4];

    uint8_t index;
    HttpRequestStatus status;

    char *verb;
    char *path;
    char *contenttype;
    uint32_t contentlength;
    uint16_t bodylength;
    
    void *handler;
    
    // TODO: Remove if not needed
    struct espconn *conn;

    char *headerbuff;
    uint16_t headerbuff_len;
    
    char *respbuff;
    uint16_t respbuff_len;

    uint32_t body_cursor;
} HttpRequest;


HttpRequest * _findrequest(struct espconn *conn);
int _deleterequest(HttpRequest *r, bool disconnect);
HttpRequest * _createrequest(struct espconn *conn, uint8_t index);
int8_t _ensurerequest(struct espconn *conn);

#endif
