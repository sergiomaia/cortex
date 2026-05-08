#include "../cortex_test.h"

void test_active_migration_register_and_execute_in_order(void);
void test_active_migration_run_only_pending(void);

CT_SUITE_BEGIN(active_migrations)
    CT_TEST(test_active_migration_register_and_execute_in_order)
    CT_TEST(test_active_migration_run_only_pending)
CT_SUITE_END()

void run_active_migration_tests(void) {
    CT_RUN_SUITE();
}
