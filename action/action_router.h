#ifndef ACTION_ROUTER_H
#define ACTION_ROUTER_H

#include "action_controller.h"

typedef struct {
    const char *method;
    const char *path;
    ActionHandler handler;
} ActionRoute;

typedef struct {
    ActionRoute *routes;
    int route_count;
} ActionRouter;

void action_router_init(ActionRouter *router);
int action_router_add_route(ActionRouter *router, const char *method, const char *path, ActionHandler handler);
ActionHandler action_router_match(ActionRouter *router, const char *method, const char *path);

#endif /* ACTION_ROUTER_H */

