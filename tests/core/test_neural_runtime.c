#include "../test_assert.h"
#include "../../core/neural_runtime.h"

void test_neural_runtime_run_returns_response(void) {
    NeuralRuntime rt = neural_runtime_init("gpt-mock");
    const char *prompt = "Hello, model!";
    const char *response = neural_run(&rt, prompt);

    ASSERT_TRUE(response != NULL);
    ASSERT_STR_EQ(response, "mock-response");
}

void test_neural_runtime_model_name_and_prompt_usage(void) {
    NeuralRuntime rt = neural_runtime_init("gpt-test-model");
    const char *prompt = "Test prompt";
    const char *response;

    /* Validate model wiring: the runtime should store the model name. */
    ASSERT_STR_EQ(rt.model.name, "gpt-test-model");

    /* We only require that neural_run returns a (mock) response and
     * accepts the prompt; specific content is mocked. */
    response = neural_run(&rt, prompt);
    ASSERT_TRUE(response != NULL);
}

void test_neural_runtime_cache_returns_cached_response_for_same_prompt(void) {
    NeuralRuntime rt = neural_runtime_init_cached("gpt-mock", 1, 16);
    const char *prompt = "Hello, model!";
    const char *r1;
    const char *r2;
    unsigned long calls_before;
    unsigned long calls_after;

    ASSERT_TRUE(rt.cache_enabled);

    r1 = neural_run(&rt, prompt);
    ASSERT_TRUE(r1 != NULL);
    calls_before = rt.compute_calls;

    r2 = neural_run(&rt, prompt);
    ASSERT_TRUE(r2 != NULL);
    calls_after = rt.compute_calls;

    /* Same response content on cache hit. */
    ASSERT_STR_EQ(r1, r2);

    /* Verify we didn't recompute. */
    ASSERT_EQ(calls_before, 1);
    ASSERT_EQ(calls_after, calls_before);
}

void test_neural_runtime_cache_hit_avoids_recomputation(void) {
    NeuralRuntime rt = neural_runtime_init_cached("gpt-mock", 1, 16);
    const char *prompt = "Prompt A";

    ASSERT_EQ(rt.compute_calls, 0);

    (void)neural_run(&rt, prompt);
    ASSERT_EQ(rt.compute_calls, 1);

    /* Second run should be served from cache. */
    (void)neural_run(&rt, prompt);
    ASSERT_EQ(rt.compute_calls, 1);
}

