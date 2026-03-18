#include <string.h>
#include <stdlib.h>

#include "action_router.h"

void action_router_init(ActionRouter *router) {
    if (!router) {
        return;
    }
    router->routes = 0;
    router->route_count = 0;
}

int action_router_add_route(ActionRouter *router, const char *method, const char *path, ActionHandler handler) {
    ActionRoute *new_routes;
    if (!router) {
        return -1;
    }
    new_routes = (ActionRoute *)realloc(router->routes, (router->route_count + 1) * sizeof(ActionRoute));
    if (!new_routes) {
        return -1;
    }
    router->routes = new_routes;
    router->routes[router->route_count].method = method;
    router->routes[router->route_count].path = path;
    router->routes[router->route_count].handler = handler;
    router->route_count += 1;
    return 0;
}

ActionHandler action_router_match(ActionRouter *router, const char *method, const char *path) {
    int i;
    if (!router || !method || !path) {
        return 0;
    }
    for (i = 0; i < router->route_count; ++i) {
        ActionRoute *r = &router->routes[i];
        if (r->method && r->path &&
            strcmp(r->method, method) == 0 &&
            strcmp(r->path, path) == 0) {
            return r->handler;
        }
    }
    return 0;
}

