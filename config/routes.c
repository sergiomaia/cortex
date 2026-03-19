#include "routes.h"

#include "../action/action_controller.h"

/* Root welcome page handler (HTML). */
static void root_welcome_controller(ActionRequest *req, ActionResponse *res) {
    (void)req;
    action_controller_render_text(res, 200, "Welcome to Cortex");
}

/* Simple health endpoint for server health checks. */
static void health_controller(ActionRequest *req, ActionResponse *res) {
    (void)req;
    action_controller_render_text(res, 200, "OK");
}

void register_routes(ActionRouter *router) {
    if (!router) {
        return;
    }

    /* Default root welcome page. */
    (void)action_router_add_route(router, "GET", "/", root_welcome_controller);

    /* Built-in health check. */
    (void)action_router_add_route(router, "GET", "/health", health_controller);
}

