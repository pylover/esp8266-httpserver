#include "common.h"
#include "router.h"


static struct httpd_route *routes;


#define MATCHROUTE(route, req) (\
    (route->verb == HTTPVERB_ANY || strcmp(route->verb, req->verb) == 0) \
    && STARTSWITH(req->path, route->pattern))


struct httpd_route * router_find(struct httpd_session *s) {
    struct httpd_route *route = routes;
    struct httpd_request *r = &s->request;
    while (true) {
        if (route->pattern == NULL){
            return NULL;
        }
        //DEBUG("Checking Route: %s\n", r->path);
        if (MATCHROUTE(route, r)) {
            //DEBUG("Route found: %s %s\r\n", route->verb, route->pattern);
            return route;
        }
        route++;
    }
}


void router_init(struct httpd_route * urls) {
    routes = urls;
}


void router_deinit(struct httpd_route ** routes_) {
    routes = NULL;
}
