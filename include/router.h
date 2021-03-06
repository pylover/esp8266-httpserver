#ifndef ROUTER_H
#define ROUTER_H

#include "session.h"


typedef err_t (*httpd_handler_t)(struct httpd_session *s);


struct httpd_route {
    char *verb;
    char *pattern;
    httpd_handler_t handler;
};


struct httpd_route * router_find(struct httpd_session *s);

#endif

