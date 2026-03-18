#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../core/core_env.h"
#include "../core/core_config.h"
#include "../core/core_logger.h"
#include "../action/action_router.h"
#include "../action/action_dispatch.h"
#include "test_assert.h"

/* Guard */
#include "../guard/guard_session.h"
#include "../guard/guard_auth.h"

/* Prototypes for tests defined in tests/core/*.c, tests/action/*.c, tests/cache/*.c, tests/forge/*.c */
void test_core_app_init_does_not_crash(void);
void test_core_logger_outputs_messages(void);
void test_core_config_loads_environment_variables(void);
void test_neural_runtime_run_returns_response(void);
void test_neural_runtime_model_name_and_prompt_usage(void);
void test_neural_runtime_cache_returns_cached_response_for_same_prompt(void);
void test_neural_runtime_cache_hit_avoids_recomputation(void);
void test_neural_prompt_replaces_variables(void);
void test_neural_prompt_handles_missing_variables(void);
void test_incident_summary_builds_prompt_and_returns_response(void);
void test_neural_memory_store_and_retrieve_by_keyword(void);
void test_neural_memory_retrieval_accuracy_overwrite(void);
void test_pulse_log_generates_entries(void);
void test_pulse_log_ai_includes_prompt_and_response(void);
void test_neural_stream_calls_callback_multiple_times(void);
void test_neural_embedding_same_input_same_vector(void);
void test_neural_embedding_different_input_different_vector(void);
void test_neural_embedding_generates_vector_from_text(void);
void test_active_model_init_sets_id_and_starts_empty(void);
void test_active_model_set_and_get_fields(void);
void test_active_model_update_existing_field(void);
void test_active_record_create_save_find(void);
void test_active_record_delete_and_data_consistency(void);
void test_active_migration_register_and_execute_in_order(void);
void test_active_migration_run_only_pending(void);
void test_active_query_where_filters_by_name(void);
void test_active_query_limit_caps_results(void);
void test_active_query_where_and_limit_chaining(void);
void test_active_query_where_filters_by_custom_field(void);
void test_active_query_execute_returns_empty_array_when_no_match(void);
void test_active_query_where_filters_by_id(void);
void test_active_relations_incident_has_many_logs(void);
void test_neural_retrieval_store_and_search_top_k(void);
void test_neural_retrieval_integrates_with_memory(void);
void test_neural_agent_register_tools_and_call_by_trigger(void);
void test_neural_agent_unknown_input_returns_null(void);
void test_pulse_metrics_counts_and_times_requests_and_ai_calls(void);
void test_pulse_trace_measures_elapsed_time_and_updates_metrics(void);
void test_action_request_parse_simple_get(void);
void test_action_response_set_formats_fields(void);
void test_action_router_register_and_match_literal_route(void);
void test_action_router_match_dynamic_incident_route(void);
void test_action_controller_receives_request_params(void);
void test_action_controller_render_json_and_text(void);
void test_action_health_endpoint_flow(void);
void test_action_incidents_show_integration(void);
void test_action_ai_incident_summary_endpoint(void);
void test_action_ai_rag_endpoint(void);
void test_llm_integration_validates_prompt_and_response(void);
void test_llm_integration_handles_missing_variables_as_empty(void);

void test_flow_queue_enqueue_and_worker_dispatch_executes_jobs(void);
void test_flow_queue_maintains_execution_order(void);

void test_cache_memory_set_and_get_key(void);
void test_cache_memory_overwrite_value(void);
void test_cache_memory_optional_expiration(void);

void test_guard_session_create_and_validate(void);
void test_guard_auth_basic_success_and_failure(void);

void test_forge_generate_controller_creates_file(void);
void test_forge_generate_model_creates_file(void);
void test_forge_generate_neural_model_creates_file(void);

void test_cli_parse_server_command(void);
void test_cli_parse_generate_controller_command(void);
void test_cli_parse_invalid_command_fails(void);
void test_cli_dispatch_generate_controller_executes_handler(void);

void test_pulse_metrics_counts_and_times_requests_and_ai_calls(void);
void test_pulse_trace_measures_elapsed_time_and_updates_metrics(void);

int test_count = 0;
int test_failures = 0;

static void test_core_env_current_defaults_to_development(void) {
    CoreEnv env = core_env_current();
    ASSERT_STR_EQ(env.name, "development");
}

static void test_core_config_load_defaults(void) {
    CoreConfig cfg = core_config_load();
    ASSERT_STR_EQ(cfg.environment, "development");
    ASSERT_EQ(cfg.port, 3000);
}

static void test_core_logger_levels(void) {
    CoreLogger logger = core_logger_init(stdout, CORE_LOG_LEVEL_INFO);

    /* Both info and error should be logged when level is INFO. */
    core_logger_info(&logger, "info message");
    core_logger_error(&logger, "error message");

    logger = core_logger_init(stdout, CORE_LOG_LEVEL_ERROR);

    /* Only error should be logged when level is ERROR. */
    core_logger_info(&logger, "info should be filtered");
    core_logger_error(&logger, "error should appear");

    /* No assertions on output, just ensure no crashes / undefined behavior. */
    ASSERT_TRUE(1);
}

static void test_action_router_add_and_match_literal(void) {
    ActionRouter router;
    ActionHandler handler;

    action_router_init(&router);

    void sample_handler(ActionRequest *req, ActionResponse *res) {
        (void)req;
        res->status = 200;
        res->body = "ok";
    }

    ASSERT_EQ(action_router_add_route(&router, "GET", "/hello", sample_handler), 0);

    handler = action_router_match(&router, "GET", "/hello");
    ASSERT_TRUE(handler != NULL);
    ASSERT_TRUE(handler == sample_handler);

    handler = action_router_match(&router, "POST", "/hello");
    ASSERT_TRUE(handler == NULL);
}

static void test_action_router_match_with_param(void) {
    ActionRouter router;
    ActionHandler handler;

    action_router_init(&router);

    void sample_handler(ActionRequest *req, ActionResponse *res) {
        (void)req;
        res->status = 200;
        res->body = "ok";
    }

    ASSERT_EQ(action_router_add_route(&router, "GET", "/users/:id", sample_handler), 0);

    handler = action_router_match(&router, "GET", "/users/123");
    ASSERT_TRUE(handler != NULL);

    handler = action_router_match(&router, "GET", "/users");
    ASSERT_TRUE(handler == NULL);
}

static void test_action_dispatch_success_and_not_found(void) {
    ActionRouter router;
    ActionRequest req;
    ActionResponse res;

    action_router_init(&router);

    void hello_handler(ActionRequest *r, ActionResponse *s) {
        (void)r;
        s->status = 200;
        s->body = "hello";
    }

    action_router_add_route(&router, "GET", "/hello", hello_handler);

    req.method = "GET";
    req.path = "/hello";
    req.body = NULL;

    res.status = 0;
    res.body = NULL;

    ASSERT_EQ(action_dispatch(&router, &req, &res), 0);
    ASSERT_EQ(res.status, 200);
    ASSERT_STR_EQ(res.body, "hello");

    req.path = "/missing";
    res.status = 0;
    res.body = NULL;

    ASSERT_EQ(action_dispatch(&router, &req, &res), -1);
    ASSERT_EQ(res.status, 404);
    ASSERT_STR_EQ(res.body, "Not Found");
}

static void run_all_tests(void) {
    test_core_env_current_defaults_to_development();
    test_core_config_load_defaults();
    test_core_logger_levels();
    test_core_app_init_does_not_crash();
    test_core_logger_outputs_messages();
    test_core_config_loads_environment_variables();
    test_neural_runtime_run_returns_response();
    test_neural_runtime_model_name_and_prompt_usage();
    test_neural_runtime_cache_returns_cached_response_for_same_prompt();
    test_neural_runtime_cache_hit_avoids_recomputation();
    test_neural_prompt_replaces_variables();
    test_neural_prompt_handles_missing_variables();
    test_incident_summary_builds_prompt_and_returns_response();
    test_neural_memory_store_and_retrieve_by_keyword();
    test_neural_memory_retrieval_accuracy_overwrite();
    test_neural_stream_calls_callback_multiple_times();
    test_neural_embedding_same_input_same_vector();
    test_neural_embedding_different_input_different_vector();
    test_neural_embedding_generates_vector_from_text();
    test_active_model_init_sets_id_and_starts_empty();
    test_active_model_set_and_get_fields();
    test_active_model_update_existing_field();
    test_pulse_log_generates_entries();
    test_pulse_log_ai_includes_prompt_and_response();
    test_llm_integration_validates_prompt_and_response();
    test_llm_integration_handles_missing_variables_as_empty();
    test_active_record_create_save_find();
    test_active_record_delete_and_data_consistency();
    test_active_migration_register_and_execute_in_order();
    test_active_migration_run_only_pending();
    test_active_query_where_filters_by_name();
    test_active_query_limit_caps_results();
    test_active_query_where_and_limit_chaining();
    test_active_query_where_filters_by_custom_field();
    test_active_query_execute_returns_empty_array_when_no_match();
    test_active_query_where_filters_by_id();
    test_active_relations_incident_has_many_logs();
    test_neural_retrieval_store_and_search_top_k();
    test_neural_retrieval_integrates_with_memory();
    test_neural_agent_register_tools_and_call_by_trigger();
    test_neural_agent_unknown_input_returns_null();
    test_pulse_metrics_counts_and_times_requests_and_ai_calls();
    test_pulse_trace_measures_elapsed_time_and_updates_metrics();
    test_action_request_parse_simple_get();
    test_action_response_set_formats_fields();
    test_action_router_register_and_match_literal_route();
    test_action_router_match_dynamic_incident_route();
    test_action_controller_receives_request_params();
    test_action_controller_render_json_and_text();
    test_action_health_endpoint_flow();
    test_action_incidents_show_integration();
    test_action_ai_incident_summary_endpoint();
    test_action_ai_rag_endpoint();
    test_action_router_add_and_match_literal();
    test_action_router_match_with_param();
    test_action_dispatch_success_and_not_found();

    test_flow_queue_enqueue_and_worker_dispatch_executes_jobs();
    test_flow_queue_maintains_execution_order();

    test_cache_memory_set_and_get_key();
    test_cache_memory_overwrite_value();
    test_cache_memory_optional_expiration();

    test_guard_session_create_and_validate();
    test_guard_auth_basic_success_and_failure();

    test_forge_generate_controller_creates_file();
    test_forge_generate_model_creates_file();
    test_forge_generate_neural_model_creates_file();

    test_cli_parse_server_command();
    test_cli_parse_generate_controller_command();
    test_cli_parse_invalid_command_fails();
    test_cli_dispatch_generate_controller_executes_handler();

    test_pulse_metrics_counts_and_times_requests_and_ai_calls();
    test_pulse_trace_measures_elapsed_time_and_updates_metrics();
}

int main(void) {
    run_all_tests();

    printf("\nTotal tests: %d\n", test_count);
    printf("Failures   : %d\n", test_failures);

    if (test_failures > 0) {
        return 1;
    }
    return 0;
}

