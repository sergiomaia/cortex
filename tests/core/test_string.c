#include "../cortex_test.h"

#include <string.h>

static void test_strlen_basic(void) {
    CT_ASSERT_EQ((int)strlen("cortex"), 6);
}

static void test_strncmp_prefix(void) {
    CT_ASSERT_EQ(strncmp("cortex", "cort", 4), 0);
    CT_ASSERT_NEQ(strncmp("cortex", "rails", 4), 0);
}

CT_SUITE_BEGIN(core_string)
    CT_TEST(test_strlen_basic)
    CT_TEST(test_strncmp_prefix)
CT_SUITE_END()

void run_core_string_tests(void) {
    CT_RUN_SUITE();
}
