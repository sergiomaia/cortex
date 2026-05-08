#include "../cortex_test.h"

void test_core_config_loads_environment_variables(void);
void test_core_config_get_db_values_from_ini(void);

CT_SUITE_BEGIN(core_config)
    CT_TEST(test_core_config_loads_environment_variables)
    CT_TEST(test_core_config_get_db_values_from_ini)
CT_SUITE_END()

void run_core_config_tests(void) {
    CT_RUN_SUITE();
}
