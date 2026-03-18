#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../core/core_env.h"
#include "../core/core_config.h"
#include "../core/core_logger.h"
#include "../action/action_router.h"
#include "../action/action_dispatch.h"

static int test_count = 0;
static int test_failures = 0;

#define ASSERT_TRUE(cond)                                                       \
    do {                                                                        \
        ++test_count;                                                           \
        if (!(cond)) {                                                          \
            ++test_failures;                                                   \
            printf("[FAIL] %s:%d: ASSERT_TRUE(%s)\n", __FILE__, __LINE__, #cond); \
        } else {                                                                \
            printf("[PASS] %s:%d: ASSERT_TRUE(%s)\n", __FILE__, __LINE__, #cond); \
        }                                                                       \
    } while (0)

#define ASSERT_EQ(a, b)                                                         \
    do {                                                                        \
        ++test_count;                                                           \
        long _va = (long)(a);                                                   \
        long _vb = (long)(b);                                                   \
        if (_va != _vb) {                                                       \
            ++test_failures;                                                   \
            printf("[FAIL] %s:%d: ASSERT_EQ(%s, %s) => %ld != %ld\n",           \
                   __FILE__, __LINE__, #a, #b, _va, _vb);                       \
        } else {                                                                \
            printf("[PASS] %s:%d: ASSERT_EQ(%s, %s)\n",                         \
                   __FILE__, __LINE__, #a, #b);                                 \
        }                                                                       \
    } while (0)

#define ASSERT_STR_EQ(a, b)                                                     \
    do {                                                                        \
        ++test_count;                                                           \
        const char *_sa = (a);                                                  \
        const char *_sb = (b);                                                  \
        if (!_sa) _sa = "";                                                     \
        if (!_sb) _sb = "";                                                     \
        if (strcmp(_sa, _sb) != 0) {                                            \
            ++test_failures;                                                   \
            printf("[FAIL] %s:%d: ASSERT_STR_EQ(%s, %s) => \"%s\" != \"%s\"\n", \
                   __FILE__, __LINE__, #a, #b, _sa, _sb);                       \
        } else {                                                                \
            printf("[PASS] %s:%d: ASSERT_STR_EQ(%s, %s)\n",                     \
                   __FILE__, __LINE__, #a, #b);                                 \
        }                                                                       \
    } while (0)

static void test_core_env_current_defaults_to_development(void) {
    CoreEnv env = core_env_current();
    ASSERT_STR_EQ(env.name, "development");
}

static void test_core_config_load_defaults(void) {
    CoreConfig cfg = core_config_load();
    ASSERT_STR_EQ(cfg.environment, "development");
    ASSERT_EQ(cfg.port, 3000);
}

static void test_core_logger_levels(void) {
    CoreLogger logger = core_logger_init(stdout, CORE_LOG_LEVEL_INFO);

    /* Both info and error should be logged when level is INFO. */
    core_logger_info(&logger, "info message");
    core_logger_error(&logger, "error message");

    logger = core_logger_init(stdout, CORE_LOG_LEVEL_ERROR);

    /* Only error should be logged when level is ERROR. */
    core_logger_info(&logger, "info should be filtered");
    core_logger_error(&logger, "error should appear");

    /* No assertions on output, just ensure no crashes / undefined behavior. */
    ASSERT_TRUE(1);
}

static void test_action_router_add_and_match_literal(void) {
    ActionRouter router;
    ActionHandler handler;

    action_router_init(&router);

    void sample_handler(ActionRequest *req, ActionResponse *res) {
        (void)req;
        res->status = 200;
        res->body = "ok";
    }

    ASSERT_EQ(action_router_add_route(&router, "GET", "/hello", sample_handler), 0);

    handler = action_router_match(&router, "GET", "/hello");
    ASSERT_TRUE(handler != NULL);
    ASSERT_TRUE(handler == sample_handler);

    handler = action_router_match(&router, "POST", "/hello");
    ASSERT_TRUE(handler == NULL);
}

static void test_action_router_match_with_param(void) {
    ActionRouter router;
    ActionHandler handler;

    action_router_init(&router);

    void sample_handler(ActionRequest *req, ActionResponse *res) {
        (void)req;
        res->status = 200;
        res->body = "ok";
    }

    ASSERT_EQ(action_router_add_route(&router, "GET", "/users/:id", sample_handler), 0);

    handler = action_router_match(&router, "GET", "/users/123");
    ASSERT_TRUE(handler != NULL);

    handler = action_router_match(&router, "GET", "/users");
    ASSERT_TRUE(handler == NULL);
}

static void test_action_dispatch_success_and_not_found(void) {
    ActionRouter router;
    ActionRequest req;
    ActionResponse res;

    action_router_init(&router);

    void hello_handler(ActionRequest *r, ActionResponse *s) {
        (void)r;
        s->status = 200;
        s->body = "hello";
    }

    action_router_add_route(&router, "GET", "/hello", hello_handler);

    req.method = "GET";
    req.path = "/hello";
    req.body = NULL;

    res.status = 0;
    res.body = NULL;

    ASSERT_EQ(action_dispatch(&router, &req, &res), 0);
    ASSERT_EQ(res.status, 200);
    ASSERT_STR_EQ(res.body, "hello");

    req.path = "/missing";
    res.status = 0;
    res.body = NULL;

    ASSERT_EQ(action_dispatch(&router, &req, &res), -1);
    ASSERT_EQ(res.status, 404);
    ASSERT_STR_EQ(res.body, "Not Found");
}

static void run_all_tests(void) {
    test_core_env_current_defaults_to_development();
    test_core_config_load_defaults();
    test_core_logger_levels();
    test_action_router_add_and_match_literal();
    test_action_router_match_with_param();
    test_action_dispatch_success_and_not_found();
}

int main(void) {
    run_all_tests();

    printf("\nTotal tests: %d\n", test_count);
    printf("Failures   : %d\n", test_failures);

    if (test_failures > 0) {
        return 1;
    }
    return 0;
}

