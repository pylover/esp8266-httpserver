#ifndef ROUTER_H
#define ROUTER_H

#include "common.h"


struct httpd_route * router_find(struct httpd_session *s);

#endif

