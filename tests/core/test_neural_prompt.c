#include "../test_assert.h"
#include "../../core/neural_prompt.h"

void test_neural_prompt_replaces_variables(void) {
    NeuralPromptVar vars[] = {
        {"name", "Alice"},
        {"id", "42"},
    };

    char out[128];
    int rc = neural_prompt_render("Hello {{name}}, id={{id}}!",
                                   vars, 2,
                                   out, (int)sizeof(out));

    ASSERT_EQ(rc, 0);
    ASSERT_STR_EQ(out, "Hello Alice, id=42!");
}

void test_neural_prompt_handles_missing_variables(void) {
    NeuralPromptVar vars[] = {
        {"name", "Alice"},
    };

    /* Missing variables should be replaced with an empty string. */
    char out[128];
    int rc = neural_prompt_render("Hello {{name}}-{{missing}}",
                                   vars, 1,
                                   out, (int)sizeof(out));

    ASSERT_EQ(rc, 0);
    ASSERT_STR_EQ(out, "Hello Alice-");
}

