#include "../test_assert.h"

#include "../../config/routes.h"
#include "../../action/action_dispatch.h"
#include "../../action/action_router.h"
#include "../../action/action_controller.h"
#include "../../action/action_request.h"
#include "../../action/action_response.h"
#include "../../core/active_record.h"

void test_root_route_returns_welcome_page(void) {
    ActionRouter router;
    ActionRequest req;
    ActionResponse res;

    action_router_init(&router);
    register_routes(&router);

    req.method = "GET";
    req.path = "/";
    req.body = NULL;

    res.status = 0;
    res.body = NULL;

    ASSERT_EQ(action_dispatch(&router, &req, &res), 0);
    ASSERT_EQ(res.status, 200);
    ASSERT_STR_EQ(res.body, "Welcome to Cortex");
}

static void register_sample_post_routes(ActionRouter *router, ActiveRecordStore *store) {
    (void)store;
    /* Minimal stub to ensure routing for /posts uses the router correctly.
     * Full scaffold behaviour is tested in forge tests. */
    void posts_index(ActionRequest *req, ActionResponse *res) {
        (void)req;
        action_controller_render_json(res, 200, "[]");
    }

    action_router_add_route(router, "GET", "/posts", posts_index);
}

void test_posts_index_route_via_router(void) {
    ActionRouter router;
    ActionRequest req;
    ActionResponse res;
    ActiveRecordStore store;

    action_router_init(&router);
    register_routes(&router);
    active_record_init(&store);
    register_sample_post_routes(&router, &store);

    req.method = "GET";
    req.path = "/posts";
    req.body = NULL;

    res.status = 0;
    res.body = NULL;

    ASSERT_EQ(action_dispatch(&router, &req, &res), 0);
    ASSERT_EQ(res.status, 200);
    ASSERT_STR_EQ(res.body, "[]");
}

