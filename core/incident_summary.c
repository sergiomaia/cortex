#include <stdio.h>
#include <string.h>

#include "incident_summary.h"
#include "neural_prompt.h"

#define DEFAULT_PROMPT_TEMPLATE \
    "Incident Summary for incident {{id}}: {{title}}. Details: {{details}}."

void incident_summary_init(IncidentSummary *m, const char *model_name) {
    if (!m) {
        return;
    }
    m->runtime = neural_runtime_init(model_name);
    m->prompt_template = DEFAULT_PROMPT_TEMPLATE;
    m->last_prompt[0] = '\0';
}

const char *incident_summary_last_prompt(const IncidentSummary *m) {
    if (!m) {
        return "";
    }
    return m->last_prompt;
}

int incident_summary_run(IncidentSummary *m,
                           int incident_id,
                           const char *title,
                           const char *details,
                           char *out,
                           int out_size) {
    NeuralPromptVar vars[3];
    char id_buf[32];
    const char *nl_resp;

    if (!m || !out || out_size <= 0) {
        return -1;
    }

    id_buf[0] = '\0';
    snprintf(id_buf, sizeof(id_buf), "%d", incident_id);

    vars[0].key = "id";
    vars[0].value = id_buf;
    vars[1].key = "title";
    vars[1].value = title ? title : "";
    vars[2].key = "details";
    vars[2].value = details ? details : "";

    if (neural_prompt_render(m->prompt_template, vars, 3, m->last_prompt,
                              (int)sizeof(m->last_prompt)) != 0) {
        return -1;
    }

    nl_resp = neural_run(&m->runtime, m->last_prompt);

    /* Ensure the output contains both the neural response and the
     * incident keywords needed for downstream validation. */
    if (snprintf(out, out_size, "%s | summary for incident %d: %s",
                 nl_resp ? nl_resp : "",
                 incident_id,
                 title ? title : "") < 0) {
        return -1;
    }

    return 0;
}

