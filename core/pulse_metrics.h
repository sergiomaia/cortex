#ifndef PULSE_METRICS_H
#define PULSE_METRICS_H

#include <stdio.h>

typedef struct {
    unsigned long request_count;
    unsigned long ai_call_count;
    double total_request_ms;
    double total_ai_ms;
} PulseMetrics;

/* Initialize all counters and timers to zero. */
void pulse_metrics_init(PulseMetrics *metrics);

/* Record a completed request with the given elapsed time in milliseconds. */
void pulse_metrics_record_request(PulseMetrics *metrics, double elapsed_ms);

/* Record a completed AI call with the given elapsed time in milliseconds. */
void pulse_metrics_record_ai_call(PulseMetrics *metrics, double elapsed_ms);

/* Print a simple human-readable snapshot of the metrics to the given stream. */
void pulse_metrics_print(const PulseMetrics *metrics, FILE *out);

#endif /* PULSE_METRICS_H */

