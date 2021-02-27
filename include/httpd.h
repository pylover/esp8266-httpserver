#ifndef HTTPD_H_
#define HTTPD_H_

#include <ip_addr.h> 
#include <espconn.h>

#define HTTPD_VERBOSE

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
#define HTTPD_MAXCONN    1
#endif


#ifndef HTTPD_TIMEOUT
#define HTTPD_TIMEOUT    5
#endif


#ifndef HTTP_REQUESTHEADER_BUFFERSIZE
#define HTTP_REQUESTHEADER_BUFFERSIZE        2 * 1024
#endif


#ifndef HTTP_RESPONSEHEADER_BUFFERSIZE
#define HTTP_RESPONSEHEADER_BUFFERSIZE       2 * 1024
#endif

#ifndef HTTP_RESPONSE_BUFFERSIZE
#define HTTP_RESPONSE_BUFFERSIZE             2 * 1024
#endif


#define HTTPVER                      "HTTP/1.1"
#define HTTPSTATUS_CONTINUE          "100 Continue"
#define HTTPSTATUS_OK                "200 Ok"
#define HTTPSTATUS_SERVERERROR       "500 Internal Server Error"
#define HTTPSTATUS_BADREQUEST        "400 Bad Request"
#define HTTPSTATUS_NOTFOUND          "404 Not Found"

#define HTTPHEADER_CONTENTTYPE_TEXT  "text/plain"
#define HTTPHEADER_CONTENTTYPE_HTML  "text/html"
#define HTTPHEADER_CONTENTTYPE_JPEG  "image/jpeg"
#define HTTPHEADER_CONTENTTYPE_ICON  "image/x-icon"
#define HTTPVERB_ANY                  NULL


#define HTTPD_OK                        0
#define HTTPD_MOREDATA                 -1 
#define HTTPD_INVALIDCONTENTTYPE       -2
#define HTTPD_INVALIDCONTENTLENGTH     -3
#define HTTPD_MAXCONNEXCEED            -4
#define HTTPD_DISCONNECT               -5
#define HTTPD_DELETECONNECTION         -6
#define HTTPD_INVALIDMAXCONN           -7
#define HTTPD_INVALIDTIMEOUT           -8
#define HTTPD_SENDMEMFULL              -9
#define HTTPS_CONNECTIONLOST          -10
#define HTTPD_INVALIDEXCEPT           -11
#define HTTPD_CONTINUE                -12


#define IP_FMT    "%d.%d.%d.%d"
#define IPPORT_FMT    IP_FMT":%d"
#define unpack_ip(ip) ip[0], ip[1], ip[2], ip[3]
#define localinfo(t) unpack_ip((t)->local_ip), (t)->local_port
#define remoteinfo(t) unpack_ip((t)->remote_ip), (t)->remote_port

#define httpd_response_text(req, status, content, content_length) \
    httpd_response(req, status, HTTPHEADER_CONTENTTYPE_TEXT, \
        content, content_length, NULL, 0)

#define httpd_response_html(req, status, content, content_length) \
    httpd_response(req, status, HTTPHEADER_CONTENTTYPE_HTML, \
        content, content_length, NULL, 0)

#define httpd_response_notok(req, status) \
    httpd_response(req, status, HTTPHEADER_CONTENTTYPE_TEXT, \
        status, strlen(status), NULL, 0)

#define httpd_response_continue(req) \
    httpd_send(req, HTTPVER" "HTTPSTATUS_CONTINUE"\r\n\r\n", 25);

#define httpd_response_notfound(req) \
    httpd_response_notok(req, HTTPSTATUS_NOTFOUND)

#define httpd_response_badrequest(req) \
    httpd_response_notok(req, HTTPSTATUS_BADREQUEST)



enum httpd_requeststatus{
    HRS_IDLE = 0,
    HRS_REQ_HEADER,
    HRS_REQ_BODY,
    HRS_RESP_HEADER,
    HRS_RESP_BODY
};


struct httpd_request{
	int remote_port;
	uint8_t remote_ip[4];

    uint8_t index;
    enum httpd_requeststatus status;

    char *path;
    char *contenttype;
    uint32_t contentlength;
    uint16_t bodylength;
    
    char *verb;
    void *handler;
    
    struct espconn *conn;

    char *req_headerbuff;
    uint16_t req_headerbuff_len;
    
    char *resp_headerbuff;
    uint16_t resp_headerbuff_len;

    char *resp_buff;
    uint16_t resp_buff_len;

    uint32_t body_cursor;
};


typedef void (*Handler)(struct httpd_request *req, char *body, 
        uint32_t body_length, uint32_t more);


struct httproute {
    char *verb;
    char *pattern;
    Handler handler;
};


struct httpd {
    struct espconn connection;
    esp_tcp esptcp;
    
    struct httpd_request **requests;
    
    struct httproute *routes;
};


void httpd_response_start(struct httpd_request *req, char *status, 
        char *content_type, uint32_t content_length, char **headers, 
        uint8_t headers_count);

err_t httpd_response_finalize(struct httpd_request *req, char *body, 
        uint32_t body_length);

err_t httpd_response(struct httpd_request *req, char *status, 
        char *content_type, char *content, uint32_t content_length, 
        char **headers, uint8_t headers_count);

err_t httpd_init(struct httpd *s, struct httproute *routes);
err_t httpd_stop();

#endif
