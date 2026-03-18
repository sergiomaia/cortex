#include "../test_assert.h"
#include "../../core/pulse_log.h"

void test_pulse_log_generates_entries(void) {
    PulseLog log;

    pulse_log_init(&log);

    ASSERT_EQ(pulse_log_append(&log, "server started"), 0);
    ASSERT_EQ(pulse_log_append(&log, "request received"), 0);

    ASSERT_EQ(log.count, 2);
    ASSERT_TRUE(log.entries[0].message != NULL);
    ASSERT_TRUE(log.entries[1].message != NULL);

    pulse_log_free(&log);
}

void test_pulse_log_ai_includes_prompt_and_response(void) {
    PulseLog log;
    const char *prompt = "Summarize incident 42";
    const char *response = "mock-response summary";

    pulse_log_init(&log);

    ASSERT_EQ(pulse_log_ai(&log, prompt, response), 0);
    ASSERT_EQ(log.count, 1);
    ASSERT_EQ(log.entries[0].kind, PULSE_LOG_KIND_AI);
    ASSERT_TRUE(log.entries[0].prompt != NULL);
    ASSERT_TRUE(log.entries[0].response != NULL);

    /* Validate AI log carries prompt and response text. */
    ASSERT_STR_EQ(log.entries[0].prompt, prompt);
    ASSERT_STR_EQ(log.entries[0].response, response);

    pulse_log_free(&log);
}

