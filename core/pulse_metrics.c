#include <stdio.h>

#include "pulse_metrics.h"

void pulse_metrics_init(PulseMetrics *metrics) {
    if (!metrics) {
        return;
    }
    metrics->request_count = 0;
    metrics->ai_call_count = 0;
    metrics->total_request_ms = 0.0;
    metrics->total_ai_ms = 0.0;
}

void pulse_metrics_record_request(PulseMetrics *metrics, double elapsed_ms) {
    if (!metrics || elapsed_ms < 0.0) {
        return;
    }
    metrics->request_count += 1;
    metrics->total_request_ms += elapsed_ms;
}

void pulse_metrics_record_ai_call(PulseMetrics *metrics, double elapsed_ms) {
    if (!metrics || elapsed_ms < 0.0) {
        return;
    }
    metrics->ai_call_count += 1;
    metrics->total_ai_ms += elapsed_ms;
}

void pulse_metrics_print(const PulseMetrics *metrics, FILE *out) {
    if (!metrics || !out) {
        return;
    }

    fprintf(out,
            "PulseMetrics: requests=%lu total_request_ms=%.3f ai_calls=%lu total_ai_ms=%.3f\n",
            metrics->request_count,
            metrics->total_request_ms,
            metrics->ai_call_count,
            metrics->total_ai_ms);
}

