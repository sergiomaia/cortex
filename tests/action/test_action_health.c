#include "../test_assert.h"
#include "../../action/action_controller.h"
#include "../../action/action_dispatch.h"

static void health_controller(ActionRequest *req, ActionResponse *res) {
    (void)req;
    action_controller_render_text(res, 200, "OK");
}

void test_action_health_endpoint_flow(void) {
    ActionRouter router;
    ActionRequest req;
    ActionResponse res;

    action_router_init(&router);
    ASSERT_EQ(action_router_add_route(&router, "GET", "/health", health_controller), 0);

    req.method = "GET";
    req.path = "/health";
    req.body = NULL;

    res.status = 0;
    res.body = NULL;

    /* Full flow: router -> controller -> response via action_dispatch. */
    ASSERT_EQ(action_dispatch(&router, &req, &res), 0);
    ASSERT_EQ(res.status, 200);
    ASSERT_STR_EQ(res.body, "OK");
}

