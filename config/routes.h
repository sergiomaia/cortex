#ifndef CONFIG_ROUTES_H
#define CONFIG_ROUTES_H

#include "../action/action_router.h"

/* Register all framework + application routes on the given router. */
void register_routes(ActionRouter *router);

/* Lightweight helpers for defining routes in a Rails-like style. */
int route_get(ActionRouter *router, const char *path, ActionHandler handler);
int route_post(ActionRouter *router, const char *path, ActionHandler handler);
int route_put(ActionRouter *router, const char *path, ActionHandler handler);
int route_delete(ActionRouter *router, const char *path, ActionHandler handler);

#endif /* CONFIG_ROUTES_H */

