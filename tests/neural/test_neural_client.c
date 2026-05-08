#include "../cortex_test.h"

void test_neural_runtime_run_returns_response(void);
void test_neural_runtime_model_name_and_prompt_usage(void);
void test_neural_runtime_cache_returns_cached_response_for_same_prompt(void);
void test_neural_runtime_cache_hit_avoids_recomputation(void);

CT_SUITE_BEGIN(neural_client)
    CT_TEST(test_neural_runtime_run_returns_response)
    CT_TEST(test_neural_runtime_model_name_and_prompt_usage)
    CT_TEST(test_neural_runtime_cache_returns_cached_response_for_same_prompt)
    CT_TEST(test_neural_runtime_cache_hit_avoids_recomputation)
CT_SUITE_END()

void run_neural_client_tests(void) {
    CT_RUN_SUITE();
}
