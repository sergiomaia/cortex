#include "../test_assert.h"
#include "../../action/action_controller.h"
#include "../../action/action_dispatch.h"

static const char *captured_method = NULL;
static const char *captured_path = NULL;
static const char *captured_body = NULL;

static void capturing_controller(ActionRequest *req, ActionResponse *res) {
    captured_method = req->method;
    captured_path = req->path;
    captured_body = req->body;

    /* Use controller helpers to render a simple text response. */
    action_controller_render_text(res, 200, "ok");
}

void test_action_controller_receives_request_params(void) {
    ActionRouter router;
    ActionRequest req;
    ActionResponse res;

    action_router_init(&router);
    ASSERT_EQ(action_router_add_route(&router, "GET", "/widgets/42", capturing_controller), 0);

    req.method = "GET";
    req.path = "/widgets/42";
    req.body = "payload";

    res.status = 0;
    res.body = NULL;

    ASSERT_EQ(action_dispatch(&router, &req, &res), 0);

    ASSERT_STR_EQ(captured_method, "GET");
    ASSERT_STR_EQ(captured_path, "/widgets/42");
    ASSERT_STR_EQ(captured_body, "payload");
    ASSERT_EQ(res.status, 200);
    ASSERT_STR_EQ(res.body, "ok");
}

void test_action_controller_render_json_and_text(void) {
    ActionResponse res;

    /* Test render_json helper. */
    res.status = 0;
    res.body = NULL;
    action_controller_render_json(&res, 201, "{\"ok\":true}");
    ASSERT_EQ(res.status, 201);
    ASSERT_STR_EQ(res.body, "{\"ok\":true}");

    /* Test render_text helper. */
    action_controller_render_text(&res, 202, "plain text");
    ASSERT_EQ(res.status, 202);
    ASSERT_STR_EQ(res.body, "plain text");
}

