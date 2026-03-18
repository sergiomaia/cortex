#ifndef PULSE_METRICS_H
#define PULSE_METRICS_H

#include <stdio.h>

/* Minimal in-memory key->counter access.
 * This is intentionally small and fixed-size to avoid allocations. */
#define PULSE_METRICS_MAX_KEYS 16
#define PULSE_METRICS_MAX_KEY_LEN 32

typedef struct {
    char key[PULSE_METRICS_MAX_KEY_LEN];
    unsigned long value;
} PulseMetricCountEntry;

typedef struct {
    unsigned long request_count;
    unsigned long ai_call_count;
    double total_request_ms;
    double total_ai_ms;

    PulseMetricCountEntry count_entries[PULSE_METRICS_MAX_KEYS];
} PulseMetrics;

/* Initialize all counters and timers to zero. */
void pulse_metrics_init(PulseMetrics *metrics);

/* Increment an in-memory counter by key. */
void pulse_metrics_inc_count(PulseMetrics *metrics, const char *key, unsigned long delta);

/* Get the current in-memory counter value by key (0 if missing). */
unsigned long pulse_metrics_get_count(const PulseMetrics *metrics, const char *key);

/* Record a completed request with the given elapsed time in milliseconds. */
void pulse_metrics_record_request(PulseMetrics *metrics, double elapsed_ms);

/* Record a completed AI call with the given elapsed time in milliseconds. */
void pulse_metrics_record_ai_call(PulseMetrics *metrics, double elapsed_ms);

/* Print a simple human-readable snapshot of the metrics to the given stream. */
void pulse_metrics_print(const PulseMetrics *metrics, FILE *out);

#endif /* PULSE_METRICS_H */

