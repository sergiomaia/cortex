#include "../cortex_test.h"

void test_active_model_init_sets_id_and_starts_empty(void);
void test_active_model_set_and_get_fields(void);
void test_active_model_update_existing_field(void);

CT_SUITE_BEGIN(active_model)
    CT_TEST(test_active_model_init_sets_id_and_starts_empty)
    CT_TEST(test_active_model_set_and_get_fields)
    CT_TEST(test_active_model_update_existing_field)
CT_SUITE_END()

void run_active_model_tests(void) {
    CT_RUN_SUITE();
}
