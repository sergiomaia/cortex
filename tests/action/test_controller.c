#include "../cortex_test.h"

void test_action_controller_receives_request_params(void);
void test_action_controller_render_json_and_text(void);
void test_action_dispatch_post_method_override_delete(void);

CT_SUITE_BEGIN(action_controller)
    CT_TEST(test_action_controller_receives_request_params)
    CT_TEST(test_action_controller_render_json_and_text)
    CT_TEST(test_action_dispatch_post_method_override_delete)
CT_SUITE_END()

void run_action_controller_tests(void) {
    CT_RUN_SUITE();
}
