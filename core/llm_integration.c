#include <stdio.h>
#include <string.h>

#include "llm_integration.h"

void llm_integration_init(LLMIntegration *llm, const char *model_name) {
    if (!llm) {
        return;
    }
    llm->runtime = neural_runtime_init(model_name);
    llm->last_prompt[0] = '\0';
}

int llm_integration_run(LLMIntegration *llm,
                         const char *prompt_template,
                         const NeuralPromptVar *vars,
                         int var_count,
                         char *out,
                         int out_size) {
    const char *resp;

    if (!llm || !prompt_template || !out || out_size <= 0) {
        return -1;
    }

    if (neural_prompt_render(prompt_template, vars, var_count,
                              llm->last_prompt, (int)sizeof(llm->last_prompt)) != 0) {
        return -1;
    }

    resp = neural_run(&llm->runtime, llm->last_prompt);
    if (!resp) {
        return -1;
    }

    if (snprintf(out, out_size, "%s", resp) < 0) {
        return -1;
    }

    return 0;
}

const char *llm_integration_last_prompt(const LLMIntegration *llm) {
    if (!llm) {
        return "";
    }
    return llm->last_prompt;
}

