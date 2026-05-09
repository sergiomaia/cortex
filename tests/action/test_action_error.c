#include <string.h>

#include "../../action/action_error.h"
#include "../../core/cortex_error.h"
#include "../cortex_test.h"

static int response_body_contains(ActionResponse res, const char *needle)
{
    if (!needle || !res.body) {
        return 0;
    }
    return strstr(res.body, needle) != NULL ? 1 : 0;
}

void test_action_handle_error_renders_json_and_clears_thread_error(void)
{
    ActionResponse res = {0};

    res.status = 200;
    res.body = "";

    cortex_clear_error();

    CT_ASSERT_EQ(ACTION_HANDLE_ERROR(&res), 0);

    CORTEX_SET_ERROR(CORTEX_ERR_ACTIVE_VALIDATION, "fixture:publish_post", "title cannot be empty");

    CT_ASSERT_EQ(ACTION_HANDLE_ERROR(&res), 1);
    CT_ASSERT_EQ(cortex_has_error(), 0);
    CT_ASSERT_EQ(res.status, 422);

    CT_ASSERT_NOT_NULL(strstr(res.content_type ? res.content_type : "", "json"));
    CT_ASSERT_EQ(response_body_contains(res, "ACTIVE_VALIDATION"), 1);
    CT_ASSERT_EQ(response_body_contains(res, "title cannot be empty"), 1);
}

CT_SUITE_BEGIN(action_error)
    CT_TEST(test_action_handle_error_renders_json_and_clears_thread_error)
CT_SUITE_END()

void run_action_error_tests(void)
{
    CT_RUN_SUITE();
}
