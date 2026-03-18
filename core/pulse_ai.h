#ifndef PULSE_AI_H
#define PULSE_AI_H

#include "pulse_log.h"

typedef struct PulseAI {
    PulseLog *log;
} PulseAI;

void pulse_ai_init(PulseAI *ai, PulseLog *log);

/* Record an AI call with structured fields. */
int pulse_ai_observe(PulseAI *ai,
                      const char *model_name,
                      const char *prompt,
                      const char *response,
                      double latency_ms);

#endif /* PULSE_AI_H */

