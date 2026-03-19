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
    ASSERT_TRUE(res.body != NULL);

    /* Welcome page should now return full HTML. */
    ASSERT_TRUE(strstr(res.body, "<h1>") != NULL);
    ASSERT_TRUE(strstr(res.body, "Welcome to Cortex.") != NULL);

    /* Placeholders should be substituted with dynamic values. */
    ASSERT_TRUE(strstr(res.body, "{{C_STANDARD}}") == NULL);
    ASSERT_TRUE(strstr(res.body, "{{CORTEX_VERSION}}") == NULL);

    /* Cortex version should include the build-time macro. */
    char needle_version[128];
    snprintf(needle_version, sizeof(needle_version), "Cortex v%s", CORTEX_VERSION);
    ASSERT_TRUE(strstr(res.body, needle_version) != NULL);
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

