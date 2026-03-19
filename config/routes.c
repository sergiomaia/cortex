#include "routes.h"

#include "../action/action_controller.h"
#include "../action/action_request.h"
#include "../action/action_response.h"

/* Default Home controller action (defined in app/controllers/home_controller.c). */
void home_index(ActionRequest *req, ActionResponse *res);

/* Simple health endpoint for server health checks. */
static void health_controller(ActionRequest *req, ActionResponse *res) {
    (void)req;
    action_controller_render_text(res, 200, "OK");
}

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

void register_routes(ActionRouter *router) {
    if (!router) {
        return;
    }

    /* Default root route – developers can change this in config/routes.c. */
    route_get(router, "/", home_index);

    /* Built-in health check. */
    route_get(router, "/health", health_controller);

    /* Application scaffold routes will be appended here by generators. */
}

