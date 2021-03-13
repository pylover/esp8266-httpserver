#ifndef QUERYSTRING_H
#define QUERYSTRING_H


#include "common.h"


typedef httpd_err_t (*httpd_querystring_cb)(struct httpd_session *s, 
        const char *name, const char *value);

void httpd_querystring_decode(char *s);

httpd_err_t httpd_querystring_parse(struct httpd_session *s, 
        httpd_querystring_cb cb);


httpd_err_t httpd_form_urlencoded_parse(struct httpd_session *s, 
        httpd_querystring_cb cb);
#endif

