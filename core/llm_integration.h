#ifndef LLM_INTEGRATION_H
#define LLM_INTEGRATION_H

#include "neural_prompt.h"
#include "neural_runtime.h"

typedef struct {
    NeuralRuntime runtime;
    char last_prompt[512];
} LLMIntegration;

void llm_integration_init(LLMIntegration *llm, const char *model_name);

/* Renders prompt via NeuralPrompt, calls the LLM (mocked by neural_run),
 * and writes the raw response into `out`. */
int llm_integration_run(LLMIntegration *llm,
                         const char *prompt_template,
                         const NeuralPromptVar *vars,
                         int var_count,
                         char *out,
                         int out_size);

const char *llm_integration_last_prompt(const LLMIntegration *llm);

#endif /* LLM_INTEGRATION_H */

