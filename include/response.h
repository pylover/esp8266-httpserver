#ifndef response_h
#define response_h

#include "common.h"


#define HTTPD_RESP_WRITE(s, d, l) rb_write(&(s)->resp_rb, (d), (l))
#define HTTPD_RESP_READ(s, d, l) rb_read(&(s)->resp_rb, (d), (l))
#define HTTPD_RESP_DRYREAD(s, d, l) rb_dryread(&(s)->resp_rb, (d), (l))
#define HTTPD_RESP_SKIP(s, l) RB_READER_SKIP(&(s)->resp_rb, (l))
#define HTTPD_RESP_LEN(s) RB_USED(&(s)->resp_rb)



#define HTTPD_RESPONSE_NOTOK(s, status) \
    httpd_response(s, status, NULL, 0, \
            HTTPHEADER_CONTENTTYPE_TEXT, status CR, \
            strlen(status) + 2, HTTPD_FLAG_CLOSE)

#define HTTPD_RESPONSE_CONTINUE(s) \
    httpd_send(s, "HTTP/1.1 "HTTPSTATUS_CONTINUE"\r\n\r\n", 25);

#define HTTPD_RESPONSE_NOTFOUND(s) \
    HTTPD_RESPONSE_NOTOK(s, HTTPSTATUS_NOTFOUND)

#define HTTPD_RESPONSE_BADREQUEST(s) \
    HTTPD_RESPONSE_NOTOK(s, HTTPSTATUS_BADREQUEST)

#define HTTPD_RESPONSE_CONFLICT(s) \
    HTTPD_RESPONSE_NOTOK(s, HTTPSTATUS_CONFLICT)

#define HTTPD_RESPONSE_INTERNALSERVERERROR(s) \
    HTTPD_RESPONSE_NOTOK(s, HTTPSTATUS_INTERNALSERVERERROR)

#define HTTPD_RESPONSE_TEXT(s, status, c, l) \
    httpd_response(s, status, NULL, 0, HTTPHEADER_CONTENTTYPE_TEXT, (c), \
            (l), HTTPD_FLAG_NONE)

#define HTTPD_RESPONSE_HTML(s, status, c, l) \
    httpd_response(s, status, NULL, 0, HTTPHEADER_CONTENTTYPE_HTML, (c), \
            (l), HTTPD_FLAG_NONE)

#define HTTPD_RESPONSE_ICON(s, status, c, l) \
    httpd_response(s, status, NULL, 0, HTTPHEADER_CONTENTTYPE_ICON, (c), \
            (l), HTTPD_FLAG_NONE)


httpd_err_t httpd_send(struct httpd_session *s, char * data, size16_t len);
void httpd_response_finalize(struct httpd_session *s, httpd_flag_t flags);
httpd_err_t httpd_response_start(struct httpd_session *s, char *status, 
        struct httpd_header *headers, uint8_t headerscount, char *contenttype, 
        uint32_t contentlen, httpd_flag_t flags);
httpd_err_t httpd_response(struct httpd_session *s, char *status,
        struct httpd_header *headers, uint8_t headerscount, char *contenttype, 
        char *content, uint32_t contentlen, httpd_flag_t flags);
#endif
