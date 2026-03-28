#include "config/routes.h"

#include "../action/action_request.h"
#include "../action/action_response.h"
#include "../action/action_controller.h"

void home_index(ActionRequest *req, ActionResponse *res);

int route_get(ActionRouter *router, const char *path, ActionHandler handler) {
    return action_router_add_route(router, "GET", path, handler);
}

int route_post(ActionRouter *router, const char *path, ActionHandler handler) {
    return action_router_add_route(router, "POST", path, handler);
}

int route_put(ActionRouter *router, const char *path, ActionHandler handler) {
    return action_router_add_route(router, "PUT", path, handler);
}

int route_delete(ActionRouter *router, const char *path, ActionHandler handler) {
    return action_router_add_route(router, "DELETE", path, handler);
}

static void health_index(ActionRequest *req, ActionResponse *res) {
    (void)req;
    action_controller_render_text(res, 200, "OK");
}

void register_routes(ActionRouter *router) {
    if (!router) {
        return;
    }
    route_get(router, "/", home_index);
    route_get(router, "/health", health_index);
}
