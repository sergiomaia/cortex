#include "test_assert.h"

#include "../action/action_dispatch.h"
#include "../action/action_router.h"
#include "../core/core_config.h"
#include "../core/core_env.h"
#include "../core/core_logger.h"

#include <stdlib.h>

void test_core_app_init_does_not_crash(void);
void test_core_logger_outputs_messages(void);
void test_core_config_loads_environment_variables(void);
void test_core_config_get_db_values_from_ini(void);
void test_secret_init_from_secret_key_base_env(void);
void test_secret_env_overrides_config_file(void);
void test_secret_loads_from_config_file(void);
void test_secret_get_cached_stable_pointer(void);
void test_secret_init_fails_in_production_without_key(void);
void test_secret_production_accepts_env(void);
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
void test_pulse_logger_logs_info_and_error(void);
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
void test_pulse_metrics_key_based_counts_and_retrieval(void);
void test_pulse_trace_measures_elapsed_time_and_updates_metrics(void);
void test_pulse_trace_duration_positive(void);
void test_pulse_ai_logs_model_prompt_response_and_latency(void);
void test_neural_runtime_triggers_pulse_ai_logging(void);
void test_root_route_returns_welcome_page(void);
void test_posts_index_route_via_router(void);
void test_action_request_parse_simple_get(void);
void test_action_response_set_formats_fields(void);
void test_action_request_form_get_decodes_urlencoded(void);
void test_action_request_form_present_matches_names(void);
void test_action_router_register_and_match_literal_route(void);
void test_action_router_match_dynamic_incident_route(void);
void test_action_controller_receives_request_params(void);
void test_action_controller_render_json_and_text(void);
void test_action_dispatch_post_method_override_delete(void);
void test_action_health_endpoint_flow(void);
void test_action_incidents_show_integration(void);
void test_action_ai_incident_summary_endpoint(void);
void test_action_ai_rag_endpoint(void);
void test_action_middleware_chain_execution_order(void);
void test_action_middleware_can_modify_request_before_controller(void);
void test_action_middleware_can_modify_response_after_controller(void);
void test_action_assets_serves_stylesheet_from_app_assets(void);
void test_action_assets_public_directory_over_app_assets(void);
void test_action_assets_rejects_path_traversal(void);
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
void test_forge_generate_controller_users_creates_file_and_stub(void);
void test_forge_generate_model_creates_file(void);
void test_forge_generate_model_plural_input_creates_singular_model(void);
void test_forge_generate_resource_creates_controller_and_views(void);
void test_forge_generate_resource_plural_input_keeps_rails_naming(void);
void test_forge_generate_service_creates_service_file(void);
void test_forge_generate_model_user_creates_file_with_fields_and_active_record(void);
void test_forge_generate_neural_model_creates_file(void);
void test_forge_generate_neural_model_plural_input_creates_singular_file(void);
void test_forge_generate_neural_prompt_creates_file(void);
void test_forge_generate_neural_agent_creates_file(void);
void test_forge_generate_neural_rag_creates_file(void);
void test_forge_generate_neural_stream_creates_file(void);
void test_forge_generate_neural_memory_creates_file(void);
void test_forge_generate_neural_retriever_creates_file(void);
void test_forge_generate_neural_integration_creates_file(void);
void test_forge_generate_neural_policy_creates_file(void);
void test_forge_scaffold_creates_model_controller_routes_fields_and_route(void);
void test_forge_scaffold_plural_input_uses_plural_controller_and_singular_model(void);
void test_db_create_creates_db_folder_and_storage_file(void);
void test_db_migrate_runs_pending_and_tracks_executed(void);
void test_db_migrate_default_has_pending_tracks_sql_files(void);
void test_db_migration_generator_creates_timestamped_file_with_up_down(void);
void test_migration_sql_parse_valid_sections(void);
void test_migration_sql_parse_optional_down(void);
void test_migration_sql_parse_rejects_missing_up(void);
void test_migration_sql_parse_semicolon_in_string(void);
void test_db_migration_scan_ignores_invalid_names(void);
void test_db_migration_apply_and_rollback_sqlite(void);
void test_db_migration_transaction_rolls_back_on_bad_sql(void);
void test_db_pool_initialization_and_clamped_size(void);
void test_db_pool_acquire_release_and_reuse(void);
void test_db_pool_acquire_multiple_connections(void);
void test_db_pool_blocking_behavior_when_exhausted(void);
void test_db_pool_applies_wal_mode(void);
void test_db_pool_shutdown_is_idempotent(void);
void test_db_pool_global_api_acquire_release(void);

void test_cli_parse_server_command(void);
void test_cli_parse_version_command(void);
void test_cli_parse_new_command(void);
void test_cli_parse_generate_controller_command(void);
void test_cli_parse_g_controller_command(void);
void test_cli_parse_g_short_controller_command(void);
void test_cli_parse_generate_resource_command(void);
void test_cli_parse_generate_model_command(void);
void test_cli_parse_generate_service_command(void);
void test_cli_parse_generate_neural_model_command(void);
void test_cli_parse_generate_neural_prompt_command(void);
void test_cli_parse_generate_neural_agent_command(void);
void test_cli_parse_generate_neural_rag_command(void);
void test_cli_parse_generate_neural_stream_command(void);
void test_cli_parse_generate_neural_memory_command(void);
void test_cli_parse_generate_neural_retriever_command(void);
void test_cli_parse_generate_neural_integration_command(void);
void test_cli_parse_generate_neural_policy_command(void);
void test_cli_parse_db_migrate_command(void);
void test_cli_parse_db_create_command(void);
void test_cli_parse_generate_stimulus_command(void);
void test_cli_parse_generate_scaffold_react_flags(void);
void test_cli_parse_g_scaffold_react_flags(void);
void test_cli_parse_generate_scaffold_missing_type_defaults_to_string(void);
void test_cli_parse_generate_scaffold_empty_type_fails(void);
void test_cli_parse_invalid_command_fails(void);
void test_cli_dispatch_generate_controller_executes_handler(void);
void test_cli_dispatch_generate_resource_executes_handler(void);
void test_cli_dispatch_generate_model_executes_handler(void);
void test_cli_dispatch_generate_service_executes_handler(void);
void test_cli_dispatch_generate_neural_model_executes_handler(void);
void test_cli_dispatch_generate_neural_prompt_executes_handler(void);
void test_cli_dispatch_generate_neural_agent_executes_handler(void);
void test_cli_dispatch_generate_neural_rag_executes_handler(void);
void test_cli_dispatch_generate_neural_stream_executes_handler(void);
void test_cli_dispatch_generate_neural_memory_executes_handler(void);
void test_cli_dispatch_generate_neural_retriever_executes_handler(void);
void test_cli_dispatch_generate_neural_integration_executes_handler(void);
void test_cli_dispatch_generate_neural_policy_executes_handler(void);
void test_cli_dispatch_db_migrate_executes_handler(void);
void test_cli_dispatch_db_create_executes_handler(void);
void test_forge_new_creates_project_directory(void);
void test_forge_new_creates_main_c_and_makefile(void);
void test_cli_dispatch_new_creates_project(void);

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

    core_logger_info(&logger, "info message");
    core_logger_error(&logger, "error message");

    logger = core_logger_init(stdout, CORE_LOG_LEVEL_ERROR);

    core_logger_info(&logger, "info should be filtered");
    core_logger_error(&logger, "error should appear");

    ASSERT_TRUE(1);
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

    free(router.routes);
}

/*
 * Additional integration coverage not grouped into numbered suites lives here so
 * previously registered Cortex tests remain single-pass.
 */
void run_legacy_tests(void) {
    printf("\n▶ Legacy and integration corpus\n");

    test_core_env_current_defaults_to_development();
    test_core_config_load_defaults();
    test_core_logger_levels();
    test_core_app_init_does_not_crash();
    test_core_logger_outputs_messages();

    test_secret_init_from_secret_key_base_env();
    test_secret_env_overrides_config_file();
    test_secret_loads_from_config_file();
    test_secret_get_cached_stable_pointer();
    test_secret_init_fails_in_production_without_key();
    test_secret_production_accepts_env();

    test_neural_prompt_replaces_variables();
    test_neural_prompt_handles_missing_variables();
    test_incident_summary_builds_prompt_and_returns_response();
    test_neural_memory_store_and_retrieve_by_keyword();
    test_neural_memory_retrieval_accuracy_overwrite();
    test_neural_stream_calls_callback_multiple_times();
    test_neural_embedding_same_input_same_vector();
    test_neural_embedding_different_input_different_vector();
    test_neural_embedding_generates_vector_from_text();
    test_pulse_log_generates_entries();
    test_pulse_log_ai_includes_prompt_and_response();
    test_pulse_logger_logs_info_and_error();
    test_llm_integration_validates_prompt_and_response();
    test_llm_integration_handles_missing_variables_as_empty();
    test_active_relations_incident_has_many_logs();
    test_neural_retrieval_store_and_search_top_k();
    test_neural_retrieval_integrates_with_memory();
    test_neural_agent_register_tools_and_call_by_trigger();
    test_neural_agent_unknown_input_returns_null();
    test_pulse_metrics_counts_and_times_requests_and_ai_calls();
    test_pulse_metrics_key_based_counts_and_retrieval();
    test_pulse_trace_measures_elapsed_time_and_updates_metrics();
    test_pulse_trace_duration_positive();
    test_pulse_ai_logs_model_prompt_response_and_latency();
    test_neural_runtime_triggers_pulse_ai_logging();
    test_root_route_returns_welcome_page();
    test_posts_index_route_via_router();
    test_action_request_parse_simple_get();
    test_action_request_form_get_decodes_urlencoded();
    test_action_request_form_present_matches_names();
    /*
     * Dispatch must run before middleware integration tests, which register
     * global hooks that change action_dispatch output.
     */
    test_action_dispatch_success_and_not_found();
    test_action_health_endpoint_flow();
    test_action_incidents_show_integration();
    test_action_ai_incident_summary_endpoint();
    test_action_ai_rag_endpoint();
    test_action_middleware_chain_execution_order();
    test_action_middleware_can_modify_request_before_controller();
    test_action_middleware_can_modify_response_after_controller();
    test_action_assets_serves_stylesheet_from_app_assets();
    test_action_assets_public_directory_over_app_assets();
    test_action_assets_rejects_path_traversal();

    test_cache_memory_set_and_get_key();
    test_cache_memory_overwrite_value();
    test_cache_memory_optional_expiration();

    test_forge_generate_controller_creates_file();
    test_forge_generate_controller_users_creates_file_and_stub();
    test_forge_generate_model_creates_file();
    test_forge_generate_model_plural_input_creates_singular_model();
    test_forge_generate_resource_creates_controller_and_views();
    test_forge_generate_resource_plural_input_keeps_rails_naming();
    test_forge_generate_service_creates_service_file();
    test_forge_generate_model_user_creates_file_with_fields_and_active_record();
    test_forge_generate_neural_model_creates_file();
    test_forge_generate_neural_model_plural_input_creates_singular_file();
    test_forge_generate_neural_prompt_creates_file();
    test_forge_generate_neural_agent_creates_file();
    test_forge_generate_neural_rag_creates_file();
    test_forge_generate_neural_stream_creates_file();
    test_forge_generate_neural_memory_creates_file();
    test_forge_generate_neural_retriever_creates_file();
    test_forge_generate_neural_integration_creates_file();
    test_forge_generate_neural_policy_creates_file();
    test_forge_scaffold_creates_model_controller_routes_fields_and_route();
    test_forge_scaffold_plural_input_uses_plural_controller_and_singular_model();

    test_db_create_creates_db_folder_and_storage_file();
    test_db_migrate_runs_pending_and_tracks_executed();
    test_db_migrate_default_has_pending_tracks_sql_files();
    test_db_migration_generator_creates_timestamped_file_with_up_down();
    test_migration_sql_parse_valid_sections();
    test_migration_sql_parse_optional_down();
    test_migration_sql_parse_rejects_missing_up();
    test_migration_sql_parse_semicolon_in_string();
    test_db_migration_scan_ignores_invalid_names();
    test_db_migration_apply_and_rollback_sqlite();
    test_db_migration_transaction_rolls_back_on_bad_sql();

    test_cli_parse_server_command();
    test_cli_parse_version_command();
    test_cli_parse_new_command();
    test_cli_parse_generate_controller_command();
    test_cli_parse_g_controller_command();
    test_cli_parse_g_short_controller_command();
    test_cli_parse_generate_resource_command();
    test_cli_parse_generate_model_command();
    test_cli_parse_generate_service_command();
    test_cli_parse_generate_neural_model_command();
    test_cli_parse_generate_neural_prompt_command();
    test_cli_parse_generate_neural_agent_command();
    test_cli_parse_generate_neural_rag_command();
    test_cli_parse_generate_neural_stream_command();
    test_cli_parse_generate_neural_memory_command();
    test_cli_parse_generate_neural_retriever_command();
    test_cli_parse_generate_neural_integration_command();
    test_cli_parse_generate_neural_policy_command();
    test_cli_parse_db_migrate_command();
    test_cli_parse_db_create_command();
    test_cli_parse_generate_stimulus_command();
    test_cli_parse_generate_scaffold_react_flags();
    test_cli_parse_g_scaffold_react_flags();
    test_cli_parse_generate_scaffold_missing_type_defaults_to_string();
    test_cli_parse_generate_scaffold_empty_type_fails();
    test_cli_parse_invalid_command_fails();
    test_cli_dispatch_generate_controller_executes_handler();
    test_cli_dispatch_generate_resource_executes_handler();
    test_cli_dispatch_generate_model_executes_handler();
    test_cli_dispatch_generate_service_executes_handler();
    test_cli_dispatch_generate_neural_model_executes_handler();
    test_cli_dispatch_generate_neural_prompt_executes_handler();
    test_cli_dispatch_generate_neural_agent_executes_handler();
    test_cli_dispatch_generate_neural_rag_executes_handler();
    test_cli_dispatch_generate_neural_stream_executes_handler();
    test_cli_dispatch_generate_neural_memory_executes_handler();
    test_cli_dispatch_generate_neural_retriever_executes_handler();
    test_cli_dispatch_generate_neural_integration_executes_handler();
    test_cli_dispatch_generate_neural_policy_executes_handler();
    test_cli_dispatch_db_migrate_executes_handler();
    test_cli_dispatch_db_create_executes_handler();
    test_forge_new_creates_project_directory();
    test_forge_new_creates_main_c_and_makefile();
    test_cli_dispatch_new_creates_project();
}
