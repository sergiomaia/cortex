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

