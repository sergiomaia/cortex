#ifndef INCIDENT_SUMMARY_H
#define INCIDENT_SUMMARY_H

#include "neural_runtime.h"

typedef struct {
    NeuralRuntime runtime;
    const char *prompt_template;
    char last_prompt[512];
} IncidentSummary;

void incident_summary_init(IncidentSummary *m, const char *model_name);

/* Renders the incident summary prompt, calls `neural_run`, and writes a
 * human-readable response into `out`.
 *
 * Returns 0 on success, -1 on error. */
int incident_summary_run(IncidentSummary *m,
                           int incident_id,
                           const char *title,
                           const char *details,
                           char *out,
                           int out_size);

const char *incident_summary_last_prompt(const IncidentSummary *m);

#endif /* INCIDENT_SUMMARY_H */

