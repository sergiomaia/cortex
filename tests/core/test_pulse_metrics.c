#include "../test_assert.h"
#include "../../core/pulse_metrics.h"
#include "../../core/pulse_trace.h"

void test_pulse_metrics_counts_and_times_requests_and_ai_calls(void) {
    PulseMetrics metrics;

    pulse_metrics_init(&metrics);

    /* Simulate three HTTP requests with varying durations. */
    pulse_metrics_record_request(&metrics, 5.0);
    pulse_metrics_record_request(&metrics, 10.5);
    pulse_metrics_record_request(&metrics, 0.0);

    ASSERT_EQ(metrics.request_count, 3UL);
    ASSERT_TRUE(metrics.total_request_ms > 15.4 && metrics.total_request_ms < 15.6);

    /* Simulate two AI calls. */
    pulse_metrics_record_ai_call(&metrics, 20.0);
    pulse_metrics_record_ai_call(&metrics, 30.0);

    ASSERT_EQ(metrics.ai_call_count, 2UL);
    ASSERT_EQ((int)metrics.total_ai_ms, 50);
}

void test_pulse_trace_measures_elapsed_time_and_updates_metrics(void) {
    PulseTrace trace;
    PulseMetrics metrics;
    double elapsed;

    pulse_metrics_init(&metrics);

    pulse_trace_start(&trace);
    /* Do a small amount of work; we do not assert exact timing,
     * only that the trace completes and returns a non-negative value. */
    {
        volatile int dummy = 0;
        int i;
        for (i = 0; i < 10000; ++i) {
            dummy += i;
        }
        (void)dummy;
    }
    pulse_trace_stop(&trace);

    elapsed = pulse_trace_elapsed_ms(&trace);
    ASSERT_TRUE(elapsed >= 0.0);

    /* Use the measured duration to update request and AI metrics. */
    pulse_metrics_record_request(&metrics, elapsed);
    pulse_metrics_record_ai_call(&metrics, elapsed);

    ASSERT_EQ(metrics.request_count, 1UL);
    ASSERT_EQ(metrics.ai_call_count, 1UL);
    ASSERT_TRUE(metrics.total_request_ms >= 0.0);
    ASSERT_TRUE(metrics.total_ai_ms >= 0.0);
}

void test_pulse_metrics_key_based_counts_and_retrieval(void) {
    PulseMetrics metrics;
    unsigned long v;

    pulse_metrics_init(&metrics);

    /* Increment multiple times and validate count retrieval by key. */
    pulse_metrics_inc_count(&metrics, "requests", 1);
    pulse_metrics_inc_count(&metrics, "requests", 2);
    pulse_metrics_inc_count(&metrics, "requests", 3);

    v = pulse_metrics_get_count(&metrics, "requests");
    ASSERT_EQ(v, 6UL);

    /* Missing keys should report 0. */
    v = pulse_metrics_get_count(&metrics, "missing");
    ASSERT_EQ(v, 0UL);

    /* Known legacy keys should remain in sync. */
    pulse_metrics_inc_count(&metrics, "request_count", 5);
    ASSERT_EQ(metrics.request_count, 5UL);
}

