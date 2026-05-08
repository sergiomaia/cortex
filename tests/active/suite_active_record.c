#include "../cortex_test.h"

void test_active_record_create_save_find(void);
void test_active_record_delete_and_data_consistency(void);

CT_SUITE_BEGIN(active_record)
    CT_TEST(test_active_record_create_save_find)
    CT_TEST(test_active_record_delete_and_data_consistency)
CT_SUITE_END()

void run_active_record_tests(void) {
    CT_RUN_SUITE();
}
