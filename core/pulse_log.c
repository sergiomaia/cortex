#include <stdlib.h>
#include <string.h>

#include "pulse_log.h"

int pulse_log_ai_observe(PulseLog *log,
                          const char *model_name,
                          const char *prompt,
                          const char *response,
                          double latency_ms);

static char *dup_string(const char *s) {
    size_t n;
    char *out;
    if (!s) {
        s = "";
    }

    n = strlen(s) + 1;
    out = (char *)malloc(n);
    if (!out) {
        return NULL;
    }
    memcpy(out, s, n);
    return out;
}

static int ensure_capacity(PulseLog *log) {
    int new_capacity;
    PulseLogEntry *new_entries;
    int i;

    if (!log) {
        return -1;
    }

    if (log->capacity == 0) {
        new_capacity = 4;
    } else if (log->count < log->capacity) {
        return 0;
    } else {
        new_capacity = log->capacity * 2;
    }

    new_entries = (PulseLogEntry *)realloc(log->entries, new_capacity * sizeof(PulseLogEntry));
    if (!new_entries) {
        return -1;
    }

    for (i = log->capacity; i < new_capacity; ++i) {
        new_entries[i].kind = PULSE_LOG_KIND_GENERIC;
        new_entries[i].message = NULL;
        new_entries[i].model_name = NULL;
        new_entries[i].latency_ms = 0.0;
        new_entries[i].prompt = NULL;
        new_entries[i].response = NULL;
    }

    log->entries = new_entries;
    log->capacity = new_capacity;
    return 0;
}

void pulse_log_init(PulseLog *log) {
    if (!log) {
        return;
    }
    log->entries = NULL;
    log->count = 0;
    log->capacity = 0;
}

void pulse_log_free(PulseLog *log) {
    int i;
    if (!log) {
        return;
    }
    for (i = 0; i < log->count; ++i) {
        free((void *)log->entries[i].message);
        /* model_name is not owned by the log */
        log->entries[i].model_name = NULL;
        log->entries[i].latency_ms = 0.0;
        free((void *)log->entries[i].prompt);
        free((void *)log->entries[i].response);
        log->entries[i].message = NULL;
        log->entries[i].model_name = NULL;
        log->entries[i].latency_ms = 0.0;
        log->entries[i].prompt = NULL;
        log->entries[i].response = NULL;
    }
    free(log->entries);
    log->entries = NULL;
    log->count = 0;
    log->capacity = 0;
}

int pulse_log_append(PulseLog *log, const char *message) {
    if (!log) {
        return -1;
    }

    if (ensure_capacity(log) != 0) {
        return -1;
    }

    log->entries[log->count].kind = PULSE_LOG_KIND_GENERIC;
    log->entries[log->count].message = dup_string(message);
    log->entries[log->count].model_name = NULL;
    log->entries[log->count].latency_ms = 0.0;
    log->entries[log->count].prompt = NULL;
    log->entries[log->count].response = NULL;

    if (!log->entries[log->count].message) {
        return -1;
    }

    log->count += 1;
    return 0;
}

int pulse_log_ai(PulseLog *log, const char *prompt, const char *response) {
    return pulse_log_ai_observe(log, NULL, prompt, response, 0.0);
}

int pulse_log_ai_observe(PulseLog *log,
                          const char *model_name,
                          const char *prompt,
                          const char *response,
                          double latency_ms) {
    if (!log || !prompt || !response) {
        return -1;
    }

    if (ensure_capacity(log) != 0) {
        return -1;
    }

    log->entries[log->count].kind = PULSE_LOG_KIND_AI;
    log->entries[log->count].message = NULL;
    log->entries[log->count].model_name = model_name;
    log->entries[log->count].latency_ms = latency_ms;
    log->entries[log->count].prompt = dup_string(prompt);
    log->entries[log->count].response = dup_string(response);

    if (!log->entries[log->count].prompt || !log->entries[log->count].response) {
        free((void *)log->entries[log->count].prompt);
        free((void *)log->entries[log->count].response);
        log->entries[log->count].prompt = NULL;
        log->entries[log->count].response = NULL;
        log->entries[log->count].model_name = NULL;
        log->entries[log->count].latency_ms = 0.0;
        return -1;
    }

    log->count += 1;
    return 0;
}

