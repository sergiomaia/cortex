#include <math.h>

#include "../test_assert.h"
#include "../../core/neural_runtime.h"
#include "../../core/pulse_ai.h"
#include "../../core/pulse_log.h"

void test_pulse_ai_logs_model_prompt_response_and_latency(void) {
    NeuralRuntime rt = neural_runtime_init("gpt-ai-mock");
    PulseLog log;
    PulseAI ai;
    const char *prompt = "Hello from AI";
    const char *resp;

    pulse_log_init(&log);
    pulse_ai_init(&ai, &log);
    neural_runtime_set_pulse_ai(&rt, &ai);

    resp = neural_run(&rt, prompt);
    ASSERT_TRUE(resp != NULL);
    ASSERT_STR_EQ(resp, "mock-response");

    ASSERT_EQ(log.count, 1);
    ASSERT_EQ(log.entries[0].kind, PULSE_LOG_KIND_AI);

    ASSERT_STR_EQ(log.entries[0].model_name, "gpt-ai-mock");
    ASSERT_STR_EQ(log.entries[0].prompt, prompt);
    ASSERT_STR_EQ(log.entries[0].response, "mock-response");

    /* Validate latency was captured (can be 0ms on very fast runs). */
    ASSERT_TRUE(log.entries[0].latency_ms >= 0.0);
    ASSERT_TRUE(!isnan(log.entries[0].latency_ms));

    pulse_log_free(&log);
}

