#ifndef NEURAL_AGENT_H
#define NEURAL_AGENT_H

/* Minimal tool-based agent.
 *
 * This is intentionally simple and rule-based:
 * - Tools are registered with a name and a trigger substring.
 * - On run, the agent picks the first tool whose trigger substring appears in
 *   the input.
 * - The agent then executes the tool function pointer.
 */

#include <stddef.h>

typedef const char *(*NeuralToolFn)(const char *input);

typedef struct {
    char *name;
    char *trigger_substring;
    NeuralToolFn fn;
} NeuralAgentTool;

typedef struct {
    NeuralAgentTool *tools;
    int count;
    int capacity;
} NeuralAgent;

void neural_agent_init(NeuralAgent *agent);
void neural_agent_free(NeuralAgent *agent);

/* Register a tool.
 * - `trigger_substring` determines when the tool should be selected.
 *   If NULL, the tool can never be selected by substring rule.
 * Returns 0 on success, -1 on error. */
int neural_agent_register_tool(NeuralAgent *agent,
                                const char *name,
                                const char *trigger_substring,
                                NeuralToolFn fn);

/* Select a tool based on input and execute it.
 * Returns the tool result (as returned by the tool function), or NULL if no
 * tool matches or on invalid inputs. */
const char *neural_agent_call_tool(const NeuralAgent *agent, const char *input);

#endif /* NEURAL_AGENT_H */

