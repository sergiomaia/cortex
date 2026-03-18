#include "../../core/neural_runtime.h"
#include "../../core/pulse_ai.h"
#include "../../core/pulse_log.h"
#include "../test_assert.h"

void test_neural_runtime_triggers_pulse_ai_logging(void) {
    NeuralRuntime rt = neural_runtime_init("gpt-integration");
    PulseLog log;
    PulseAI ai;
    const char *prompt = "Integration prompt";
    const char *resp;

    pulse_log_init(&log);
    pulse_ai_init(&ai, &log);
    neural_runtime_set_pulse_ai(&rt, &ai);

    resp = neural_run(&rt, prompt);
    ASSERT_TRUE(resp != NULL);
    ASSERT_STR_EQ(resp, "mock-response");

    /* Integration assertion: NeuralRuntime->neural_run must trigger PulseAI. */
    ASSERT_EQ(log.count, 1);
    ASSERT_EQ(log.entries[0].kind, PULSE_LOG_KIND_AI);
    ASSERT_STR_EQ(log.entries[0].model_name, "gpt-integration");
    ASSERT_STR_EQ(log.entries[0].prompt, prompt);
    ASSERT_STR_EQ(log.entries[0].response, "mock-response");
    ASSERT_TRUE(log.entries[0].latency_ms >= 0.0);

    pulse_log_free(&log);
}

