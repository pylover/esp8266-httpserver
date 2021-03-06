#include "router.h"

static struct httpd_route **routes;


struct httpd_route * router_find(struct httpd_session *s) {
    /* Not found */
    return NULL;
}


void router_init(struct httpd_route ** routes_) {
    routes = routes;
}


void router_deinit(struct httpd_route ** routes_) {
    routes = NULL;
}
