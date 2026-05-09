#include "cortex_test.h"

#include <stdio.h>

void run_db_connection_tests(void);
void run_db_pool_tests(void);
void run_active_model_tests(void);
void run_active_record_tests(void);
void run_active_query_tests(void);
void run_active_migration_tests(void);
void run_core_config_tests(void);
void run_core_string_tests(void);
void run_cortex_error_tests(void);
void run_action_router_tests(void);
void run_action_controller_tests(void);
void run_action_response_tests(void);
void run_action_error_tests(void);
void run_guard_tests(void);
void run_flow_jobs_tests(void);
void run_neural_client_tests(void);
void run_legacy_tests(void);

int main(void) {
    printf("Cortex Test Runner\n");
    printf("==================\n");

    run_db_connection_tests();

    run_active_model_tests();
    run_active_record_tests();
    run_active_query_tests();
    run_active_migration_tests();

    run_core_config_tests();
    run_core_string_tests();
    run_cortex_error_tests();

    run_action_router_tests();
    run_action_controller_tests();
    run_action_response_tests();
    run_action_error_tests();

    run_guard_tests();
    run_db_pool_tests();
    run_flow_jobs_tests();
    run_neural_client_tests();

    run_legacy_tests();

    CT_REPORT();

    return CT_EXIT_CODE();
}
