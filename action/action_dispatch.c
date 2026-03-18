#include "action_dispatch.h"

int action_dispatch(ActionRouter *router, ActionRequest *req, ActionResponse *res) {
    ActionHandler handler;
    if (!router || !req || !res) {
        return -1;
    }
    handler = action_router_match(router, req->method, req->path);
    if (!handler) {
        action_response_set(res, 404, "Not Found");
        return -1;
    }
    handler(req, res);
    return 0;
}

