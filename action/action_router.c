#include <string.h>
#include <stdlib.h>

#include "action_router.h"

static int paths_match(const char *route_path, const char *request_path) {
    /* Very small router: supports literal segments and :param segments. */
    const char *rp = route_path;
    const char *qp = request_path;

    if (!rp || !qp) {
        return 0;
    }

    while (*rp != '\0' && *qp != '\0') {
        if (*rp == ':') {
            /* Skip parameter name in route until next slash or end. */
            while (*rp != '\0' && *rp != '/') {
                rp++;
            }
            /* Consume request path until next slash or end. */
            while (*qp != '\0' && *qp != '/') {
                qp++;
            }
        } else {
            if (*rp != *qp) {
                return 0;
            }
            rp++;
            qp++;
        }
    }

    /* Allow trailing parameter in route (e.g., "/users/:id"). */
    if (*rp == ':' && *qp == '\0') {
        return 1;
    }

    return (*rp == '\0' && *qp == '\0');
}

void action_router_init(ActionRouter *router) {
    if (!router) {
        return;
    }
    router->routes = 0;
    router->route_count = 0;
    router->route_capacity = 0;
}

int action_router_add_route(ActionRouter *router, const char *method, const char *path, ActionHandler handler) {
    ActionRoute *routes;
    if (!router || !method || !path || !handler) {
        return -1;
    }

    if (router->route_capacity == 0) {
        router->route_capacity = 4;
        router->routes = (ActionRoute *)malloc(router->route_capacity * sizeof(ActionRoute));
        if (!router->routes) {
            router->route_capacity = 0;
            return -1;
        }
    } else if (router->route_count >= router->route_capacity) {
        int new_capacity = router->route_capacity * 2;
        routes = (ActionRoute *)realloc(router->routes, new_capacity * sizeof(ActionRoute));
        if (!routes) {
            return -1;
        }
        router->routes = routes;
        router->route_capacity = new_capacity;
    }

    routes = router->routes;
    routes[router->route_count].method = method;
    routes[router->route_count].path = path;
    routes[router->route_count].handler = handler;
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
        if (!r->method || !r->path || !r->handler) {
            continue;
        }
        if (strcmp(r->method, method) == 0 && paths_match(r->path, path)) {
            return r->handler;
        }
    }
    return 0;
}

