#include <stdio.h>
#include <string.h>

#include "pulse_metrics.h"

static PulseMetricCountEntry *pulse_metrics_find_entry(PulseMetrics *metrics, const char *key) {
    int i;

    if (!metrics || !key) {
        return NULL;
    }

    for (i = 0; i < PULSE_METRICS_MAX_KEYS; ++i) {
        if (metrics->count_entries[i].key[0] != '\0' &&
            strcmp(metrics->count_entries[i].key, key) == 0) {
            return &metrics->count_entries[i];
        }
    }

    return NULL;
}

static PulseMetricCountEntry *pulse_metrics_find_or_create_entry(PulseMetrics *metrics, const char *key) {
    int i;
    PulseMetricCountEntry *entry;

    if (!metrics || !key) {
        return NULL;
    }

    entry = pulse_metrics_find_entry(metrics, key);
    if (entry) {
        return entry;
    }

    for (i = 0; i < PULSE_METRICS_MAX_KEYS; ++i) {
        if (metrics->count_entries[i].key[0] == '\0') {
            /* Create a new entry. */
            strncpy(metrics->count_entries[i].key, key, PULSE_METRICS_MAX_KEY_LEN - 1);
            metrics->count_entries[i].key[PULSE_METRICS_MAX_KEY_LEN - 1] = '\0';
            metrics->count_entries[i].value = 0;
            return &metrics->count_entries[i];
        }
    }

    /* Table full; drop the update. */
    return NULL;
}

void pulse_metrics_inc_count(PulseMetrics *metrics, const char *key, unsigned long delta) {
    PulseMetricCountEntry *entry;

    if (!metrics || !key || delta == 0) {
        return;
    }

    /* Keep legacy typed counters in sync for the known keys. */
    if (strcmp(key, "request_count") == 0) {
        metrics->request_count += delta;
    } else if (strcmp(key, "ai_call_count") == 0) {
        metrics->ai_call_count += delta;
    }

    entry = pulse_metrics_find_or_create_entry(metrics, key);
    if (!entry) {
        return;
    }

    entry->value += delta;
}

unsigned long pulse_metrics_get_count(const PulseMetrics *metrics, const char *key) {
    PulseMetricCountEntry *entry;

    if (!metrics || !key) {
        return 0;
    }

    entry = pulse_metrics_find_entry((PulseMetrics *)metrics, key);
    if (!entry) {
        return 0;
    }

    return entry->value;
}

void pulse_metrics_init(PulseMetrics *metrics) {
    if (!metrics) {
        return;
    }
    metrics->request_count = 0;
    metrics->ai_call_count = 0;
    metrics->total_request_ms = 0.0;
    metrics->total_ai_ms = 0.0;

    for (int i = 0; i < PULSE_METRICS_MAX_KEYS; ++i) {
        metrics->count_entries[i].key[0] = '\0';
        metrics->count_entries[i].value = 0;
    }
}

void pulse_metrics_record_request(PulseMetrics *metrics, double elapsed_ms) {
    if (!metrics || elapsed_ms < 0.0) {
        return;
    }
    /* Typed counter and keyed counter share the same in-memory value. */
    pulse_metrics_inc_count(metrics, "request_count", 1);
    metrics->total_request_ms += elapsed_ms;
}

void pulse_metrics_record_ai_call(PulseMetrics *metrics, double elapsed_ms) {
    if (!metrics || elapsed_ms < 0.0) {
        return;
    }
    pulse_metrics_inc_count(metrics, "ai_call_count", 1);
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

