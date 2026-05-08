#include "../cortex_test.h"

void test_action_response_set_formats_fields(void);

CT_SUITE_BEGIN(action_response)
    CT_TEST(test_action_response_set_formats_fields)
CT_SUITE_END()

void run_action_response_tests(void) {
    CT_RUN_SUITE();
}
