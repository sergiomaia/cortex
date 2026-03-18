#include "pulse_ai.h"

void pulse_ai_init(PulseAI *ai, PulseLog *log) {
    if (!ai) {
        return;
    }
    ai->log = log;
}

int pulse_ai_observe(PulseAI *ai,
                      const char *model_name,
                      const char *prompt,
                      const char *response,
                      double latency_ms) {
    if (!ai || !ai->log) {
        return -1;
    }

    return pulse_log_ai_observe(ai->log, model_name, prompt, response, latency_ms);
}

