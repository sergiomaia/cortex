#include <string.h>

#include "../test_assert.h"
#include "../../core/llm_integration.h"

void test_llm_integration_validates_prompt_and_response(void) {
    LLMIntegration llm;
    NeuralPromptVar vars[2];
    char out[256];

    llm_integration_init(&llm, "gpt-mock");

    vars[0].key = "id";
    vars[0].value = "99";
    vars[1].key = "title";
    vars[1].value = "Router failure";

    ASSERT_EQ(llm_integration_run(&llm,
                                    "Summarize incident {{id}}: {{title}}",
                                    vars,
                                    2,
                                    out,
                                    (int)sizeof(out)),
              0);

    /* Validate prompt was rendered and "sent" to the LLM. */
    ASSERT_STR_EQ(llm_integration_last_prompt(&llm),
                   "Summarize incident 99: Router failure");

    /* Validate response is received (mock-response is acceptable). */
    ASSERT_TRUE(out[0] != '\0');
    ASSERT_TRUE(strstr(out, "mock-response") != NULL);
}

void test_llm_integration_handles_missing_variables_as_empty(void) {
    LLMIntegration llm;
    NeuralPromptVar vars[1];
    char out[256];

    llm_integration_init(&llm, "gpt-mock");

    vars[0].key = "id";
    vars[0].value = "7";

    ASSERT_EQ(llm_integration_run(&llm,
                                    "Incident {{id}}: {{missing}}",
                                    vars,
                                    1,
                                    out,
                                    (int)sizeof(out)),
              0);

    /* Missing variables should become empty string. */
    ASSERT_STR_EQ(llm_integration_last_prompt(&llm),
                   "Incident 7: ");
}

