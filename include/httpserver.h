// TODO: Rename to httpd
#ifndef HTTPSERVER_H_
#define HTTPSERVER_H_

#include "request.h"

#include <ip_addr.h> 
#include <espconn.h>

#ifndef HTTPSERVER_PORT
#define HTTPSERVER_PORT    80
#endif


#ifndef HTTPSERVER_MAXCONN
#define HTTPSERVER_MAXCONN    1
#endif


#ifndef HTTPSERVER_TIMEOUT
#define HTTPSERVER_TIMEOUT    1
#endif


#ifndef HTTP_HEADER_BUFFER_SIZE
#define HTTP_HEADER_BUFFER_SIZE        4 * 1024
#endif

#define HTTPSTATUS_SERVERERROR       "500 Internal Server Error"
#define HTTPSTATUS_BADREQUEST        "400 Bad Request"
#define HTTPSTATUS_NOTFOUND          "404 Not Found"
#define HTTPSTATUS_OK                "200 Ok"

#define HTTPHEADER_CONTENTTYPE_TEXT  "text/plain"
#define HTTPHEADER_CONTENTTYPE_HTML  "text/html"
#define HTTPHEADER_CONTENTTYPE_JPEG  "image/jpeg"
#define HTTPVERB_ANY                  NULL
#define HTTP_RESPONSE_BUFFER_SIZE    2 * 1024

#define OK             0
#define MORE          -2
#define ERR_MAXCONN   -3


#define IP_FMT    "%d.%d.%d.%d"
#define IPPORT_FMT    IP_FMT":%d"
#define unpack_ip(ip) ip[0], ip[1], ip[2], ip[3]
#define localinfo(t) unpack_ip((t)->local_ip), (t)->local_port
#define remoteinfo(t) unpack_ip((t)->remote_ip), (t)->remote_port

#define httpserver_response_text(req, status, content, content_length) \
    httpserver_response(req, status, HTTPHEADER_CONTENTTYPE_TEXT, \
        content, content_length, NULL, 0)

#define httpserver_response_html(req, status, content, content_length) \
    httpserver_response(req, status, HTTPHEADER_CONTENTTYPE_HTML, \
        content, content_length, NULL, 0)

#define httpserver_response_notok(req, status) \
    httpserver_response(req, status, HTTPHEADER_CONTENTTYPE_TEXT, \
        status, strlen(status), NULL, 0)

#define httpserver_response_continue(req) \
    httpserver_response(req, 100, HTTPHEADER_CONTENTTYPE_TEXT, \
        status, strlen(status), NULL, 0)

#define httpserver_response_notfound(req) \
    httpserver_response_notok(req, HTTPSTATUS_NOTFOUND)

#define httpserver_response_badrequest(req) \
    httpserver_response_notok(req, HTTPSTATUS_BADREQUEST)


#define startswith(str, searchfor) \
    (strncmp(searchfor, str, strlen(searchfor)) == 0)


#define matchroute(route, req) (\
    (route->verb == HTTPVERB_ANY || strcmp(route->verb, req->verb) == 0) \
    && startswith(req->path, route->pattern) \
)


typedef void (*Handler)(Request *req, char *body, uint32_t body_length, 
        uint32_t more);


typedef struct {
    char *verb;
    char *pattern;
    Handler handler;
} HttpRoute;


typedef enum {
    HSE_MOREDATA = 0,
    HSE_INVALIDCONTENTTYPE = -1,
    HSE_INVALIDCONTENTLENGTH = -2,

} HttpServerError;


typedef struct {
    // TODO: Malloc these two 
    struct espconn connection;
    esp_tcp esptcp;
    
    HttpRequest **requests;
    uint8_t requestscount;

    HttpRoute *routes;
} HttpServer;


int httpserver_response_start(HttpRequest *req, char *status, 
        char *content_type, uint32_t content_length, char **headers, 
        uint8_t headers_count);

int httpserver_response_finalize(HttpRequest *req, char *body, 
        uint32_t body_length);

int httpserver_response(HttpRequest *req, char *status, char *content_type, 
        char *content, uint32_t content_length, char **headers, 
        uint8_t headers_count);

int httpserver_init(HttpServer *s);
void httpserver_stop();

#endif
