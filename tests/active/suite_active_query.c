#include "../cortex_test.h"

void test_active_query_where_filters_by_name(void);
void test_active_query_limit_caps_results(void);
void test_active_query_where_and_limit_chaining(void);
void test_active_query_where_filters_by_custom_field(void);
void test_active_query_execute_returns_empty_array_when_no_match(void);
void test_active_query_where_filters_by_id(void);

CT_SUITE_BEGIN(active_query)
    CT_TEST(test_active_query_where_filters_by_name)
    CT_TEST(test_active_query_limit_caps_results)
    CT_TEST(test_active_query_where_and_limit_chaining)
    CT_TEST(test_active_query_where_filters_by_custom_field)
    CT_TEST(test_active_query_execute_returns_empty_array_when_no_match)
    CT_TEST(test_active_query_where_filters_by_id)
CT_SUITE_END()

void run_active_query_tests(void) {
    CT_RUN_SUITE();
}
