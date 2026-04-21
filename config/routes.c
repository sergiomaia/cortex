#include "config/routes.h"
#include "../action/action_request.h"
#include "../action/action_response.h"
#include "../action/action_controller.h"

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

__attribute__((weak)) void pages_index(ActionRequest *req, ActionResponse *res) { (void)req; action_controller_render_text(res, 500, "Missing controller handler: pages_index"); }
__attribute__((weak)) void pages_show(ActionRequest *req, ActionResponse *res) { (void)req; action_controller_render_text(res, 500, "Missing controller handler: pages_show"); }
__attribute__((weak)) void pages_new(ActionRequest *req, ActionResponse *res) { (void)req; action_controller_render_text(res, 500, "Missing controller handler: pages_new"); }
__attribute__((weak)) void pages_edit(ActionRequest *req, ActionResponse *res) { (void)req; action_controller_render_text(res, 500, "Missing controller handler: pages_edit"); }
__attribute__((weak)) void pages_create(ActionRequest *req, ActionResponse *res) { (void)req; action_controller_render_text(res, 500, "Missing controller handler: pages_create"); }
__attribute__((weak)) void pages_update(ActionRequest *req, ActionResponse *res) { (void)req; action_controller_render_text(res, 500, "Missing controller handler: pages_update"); }
__attribute__((weak)) void pages_delete(ActionRequest *req, ActionResponse *res) { (void)req; action_controller_render_text(res, 500, "Missing controller handler: pages_delete"); }
void home_index(ActionRequest *req, ActionResponse *res);

void app_register_routes(ActionRouter *router) {
    if (!router) return;
    route_get(router, "/", home_index);
    route_get(router, "/pages", pages_index);
    route_get(router, "/pages.json", pages_index);
    route_get(router, "/pages/new", pages_new);
    route_get(router, "/pages/:id/edit", pages_edit);
    route_get(router, "/pages/:id", pages_show);
    route_get(router, "/pages/:id.json", pages_show);
    route_post(router, "/pages", pages_create);
    route_post(router, "/pages/:id", pages_update);
    route_put(router, "/pages/:id", pages_update);
    route_delete(router, "/pages/:id", pages_delete);
}

void register_routes(ActionRouter *router) {
    app_register_routes(router);
}
