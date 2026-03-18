#include "../test_assert.h"
#include "../../action/action_router.h"
#include "../../action/action_dispatch.h"
#include "../../action/action_middleware.h"

static char execution_order[8];
static int execution_pos;

static void reset_execution_order(void) {
    int i;
    for (i = 0; i < (int)(sizeof(execution_order) / sizeof(execution_order[0])); ++i) {
        execution_order[i] = '\0';
    }
    execution_pos = 0;
}

static void record_step(char c) {
    if (execution_pos < (int)(sizeof(execution_order) / sizeof(execution_order[0]) - 1)) {
        execution_order[execution_pos++] = c;
        execution_order[execution_pos] = '\0';
    }
}

static void mw_record_before_and_after(ActionRequest *req, ActionResponse *res, ActionMiddlewareNext next) {
    (void)req;
    (void)res;
    record_step('A');
    next();
    record_step('C');
}

static void mw_record_before(ActionRequest *req, ActionResponse *res, ActionMiddlewareNext next) {
    (void)req;
    (void)res;
    record_step('B');
    next();
}

static void controller_record(ActionRequest *req, ActionResponse *res) {
    (void)req;
    (void)res;
    record_step('X');
}

void test_action_middleware_chain_execution_order(void) {
    ActionRouter router;
    ActionRequest req;
    ActionResponse res;

    action_router_init(&router);
    action_middleware_clear();
    ASSERT_EQ(action_middleware_add(mw_record_before), 0);
    ASSERT_EQ(action_middleware_add(mw_record_before_and_after), 0);

    ASSERT_EQ(action_router_add_route(&router, "GET", "/order", controller_record), 0);

    reset_execution_order();

    req.method = "GET";
    req.path = "/order";
    req.body = NULL;

    res.status = 0;
    res.body = NULL;

    ASSERT_EQ(action_dispatch(&router, &req, &res), 0);

    ASSERT_STR_EQ(execution_order, "BAXC");
}

static const char *captured_body = NULL;

static void mw_modify_request(ActionRequest *req, ActionResponse *res, ActionMiddlewareNext next) {
    (void)res;
    req->body = "modified-by-middleware";
    next();
}

static void capturing_controller(ActionRequest *req, ActionResponse *res) {
    captured_body = req->body;
    res->status = 200;
    res->body = "original-body";
}

void test_action_middleware_can_modify_request_before_controller(void) {
    ActionRouter router;
    ActionRequest req;
    ActionResponse res;

    action_router_init(&router);
    action_middleware_clear();
    ASSERT_EQ(action_middleware_add(mw_modify_request), 0);

    ASSERT_EQ(action_router_add_route(&router, "POST", "/modify", capturing_controller), 0);

    req.method = "POST";
    req.path = "/modify";
    req.body = "original";

    res.status = 0;
    res.body = NULL;

    captured_body = NULL;

    ASSERT_EQ(action_dispatch(&router, &req, &res), 0);

    ASSERT_STR_EQ(captured_body, "modified-by-middleware");
    ASSERT_EQ(res.status, 200);
    ASSERT_STR_EQ(res.body, "original-body");
}

static void mw_modify_response_after(ActionRequest *req, ActionResponse *res, ActionMiddlewareNext next) {
    (void)req;
    next();
    res->status = 201;
    res->body = "modified-response";
}

static void simple_controller(ActionRequest *req, ActionResponse *res) {
    (void)req;
    res->status = 200;
    res->body = "controller-body";
}

void test_action_middleware_can_modify_response_after_controller(void) {
    ActionRouter router;
    ActionRequest req;
    ActionResponse res;

    action_router_init(&router);
    action_middleware_clear();
    ASSERT_EQ(action_middleware_add(mw_modify_response_after), 0);

    ASSERT_EQ(action_router_add_route(&router, "GET", "/after", simple_controller), 0);

    req.method = "GET";
    req.path = "/after";
    req.body = NULL;

    res.status = 0;
    res.body = NULL;

    ASSERT_EQ(action_dispatch(&router, &req, &res), 0);

    ASSERT_EQ(res.status, 201);
    ASSERT_STR_EQ(res.body, "modified-response");
}

