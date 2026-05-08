#include "../../action/action_controller.h"
#include "../../action/action_router.h"
#include "../cortex_test.h"

#include <stdlib.h>

static void dummy_handler_posts(ActionRequest *req, ActionResponse *res) {
    (void)req;
    (void)res;
}

static void test_route_match_exact(void) {
    ActionRouter router;
    ActionHandler handler;

    action_router_init(&router);
    CT_ASSERT_EQ(action_router_add_route(&router, "GET", "/posts", dummy_handler_posts), 0);

    handler = action_router_match(&router, "GET", "/posts");
    CT_ASSERT_NOT_NULL(handler);
    CT_ASSERT(handler == dummy_handler_posts);

    handler = action_router_match(&router, "GET", "/comments");
    CT_ASSERT_NULL(handler);

    free(router.routes);
}

static void test_route_match_param(void) {
    ActionRouter router;
    ActionHandler handler;

    action_router_init(&router);
    CT_ASSERT_EQ(action_router_add_route(&router, "GET", "/posts/:id", dummy_handler_posts), 0);

    handler = action_router_match(&router, "GET", "/posts/42");
    CT_ASSERT_NOT_NULL(handler);

    handler = action_router_match(&router, "GET", "/posts");
    CT_ASSERT_NULL(handler);

    free(router.routes);
}

static void test_route_no_match(void) {
    ActionRouter router;

    action_router_init(&router);
    CT_ASSERT_EQ(action_router_add_route(&router, "GET", "/posts", dummy_handler_posts), 0);

    CT_ASSERT_NULL(action_router_match(&router, "GET", "/comments"));

    free(router.routes);
}

static void test_method_mismatch(void) {
    ActionRouter router;

    action_router_init(&router);
    CT_ASSERT_EQ(action_router_add_route(&router, "GET", "/posts", dummy_handler_posts), 0);

    CT_ASSERT_NULL(action_router_match(&router, "POST", "/posts"));

    free(router.routes);
}

CT_SUITE_BEGIN(action_router)
    CT_TEST(test_route_match_exact)
    CT_TEST(test_route_match_param)
    CT_TEST(test_route_no_match)
    CT_TEST(test_method_mismatch)
CT_SUITE_END()

void run_action_router_tests(void) {
    CT_RUN_SUITE();
}
