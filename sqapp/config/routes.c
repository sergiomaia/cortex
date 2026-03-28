#include "config/routes.h"
#include "../action/action_request.h"
#include "../action/action_response.h"

void home_index(ActionRequest *req, ActionResponse *res);
void users_index(ActionRequest *req, ActionResponse *res);

void app_register_routes(ActionRouter *router) {
    if (!router) return;
    route_get(router, "/", home_index);
    route_get(router, "/users", users_index);
}
