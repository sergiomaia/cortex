#include "../test_assert.h"
#include "../../action/action_router.h"

static void dummy_handler_ok(ActionRequest *req, ActionResponse *res) {
    (void)req;
    (void)res;
}

static void dummy_handler_incident(ActionRequest *req, ActionResponse *res) {
    (void)req;
    (void)res;
}

void test_action_router_register_and_match_literal_route(void) {
    ActionRouter router;
    ActionHandler handler;

    action_router_init(&router);

    ASSERT_EQ(action_router_add_route(&router, "GET", "/status", dummy_handler_ok), 0);

    handler = action_router_match(&router, "GET", "/status");
    ASSERT_TRUE(handler != NULL);
    ASSERT_TRUE(handler == dummy_handler_ok);

    /* Negative check: different path should not match. */
    handler = action_router_match(&router, "GET", "/other");
    ASSERT_TRUE(handler == NULL);
}

void test_action_router_match_dynamic_incident_route(void) {
    ActionRouter router;
    ActionHandler handler;

    action_router_init(&router);

    ASSERT_EQ(action_router_add_route(&router, "GET", "/incidents/:id", dummy_handler_incident), 0);

    handler = action_router_match(&router, "GET", "/incidents/123");
    ASSERT_TRUE(handler != NULL);
    ASSERT_TRUE(handler == dummy_handler_incident);

    /* Different prefix/path should not match. */
    handler = action_router_match(&router, "GET", "/incidents");
    ASSERT_TRUE(handler == NULL);

    handler = action_router_match(&router, "GET", "/incidents/123/details");
    ASSERT_TRUE(handler == NULL);
}

