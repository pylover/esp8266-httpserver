#ifndef QUERYSTRING_H
#define QUERYSTRING_H


#include "session.h"


typedef void (*httpd_querystring_cb)(struct httpd_session *s, 
        const char *name, const char *value);

void httpd_querystring_parse(struct httpd_session *s, 
        httpd_querystring_cb cb);


#endif

