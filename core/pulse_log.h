#ifndef PULSE_LOG_H
#define PULSE_LOG_H

typedef enum {
    PULSE_LOG_KIND_GENERIC,
    PULSE_LOG_KIND_AI
} PulseLogKind;

typedef struct {
    PulseLogKind kind;
    const char *message;
    const char *prompt;
    const char *response;
} PulseLogEntry;

typedef struct {
    PulseLogEntry *entries;
    int count;
    int capacity;
} PulseLog;

void pulse_log_init(PulseLog *log);
void pulse_log_free(PulseLog *log);

/* Append a generic log message. Returns 0 on success, -1 on failure. */
int pulse_log_append(PulseLog *log, const char *message);

/* Append an AI log with prompt and response.
 * Both prompt and response must be non-NULL.
 * Returns 0 on success, -1 on failure. */
int pulse_log_ai(PulseLog *log, const char *prompt, const char *response);

#endif /* PULSE_LOG_H */

