#include <string.h>

#include "../test_assert.h"
#include "../../core/neural_agent.h"

static const char *tool_greet(const char *input) {
    (void)input;
    return "hi";
}

static const char *tool_bye(const char *input) {
    (void)input;
    return "goodbye";
}

static const char *tool_unused(const char *input) {
    (void)input;
    return "unused";
}

void test_neural_agent_register_tools_and_call_by_trigger(void) {
    NeuralAgent agent;
    const char *result;

    neural_agent_init(&agent);

    ASSERT_EQ(neural_agent_register_tool(&agent, "greet", "hello", tool_greet), 0);
    ASSERT_EQ(neural_agent_register_tool(&agent, "bye", "bye", tool_bye), 0);

    result = neural_agent_call_tool(&agent, "hello there");
    ASSERT_TRUE(result != NULL);
    ASSERT_STR_EQ(result, "hi");

    result = neural_agent_call_tool(&agent, "please bye now");
    ASSERT_TRUE(result != NULL);
    ASSERT_STR_EQ(result, "goodbye");

    neural_agent_free(&agent);
}

void test_neural_agent_unknown_input_returns_null(void) {
    NeuralAgent agent;
    const char *result;

    neural_agent_init(&agent);

    ASSERT_EQ(neural_agent_register_tool(&agent, "noop", NULL, tool_unused), 0);

    result = neural_agent_call_tool(&agent, "something else");
    ASSERT_TRUE(result == NULL);

    neural_agent_free(&agent);
}

