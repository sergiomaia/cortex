#include <stdlib.h>
#include <string.h>

#include "neural_agent.h"

static char *dup_string(const char *s) {
    size_t n;
    char *out;

    if (!s) {
        return NULL;
    }
    n = strlen(s) + 1;
    out = (char *)malloc(n);
    if (!out) {
        return NULL;
    }
    memcpy(out, s, n);
    return out;
}

static int ensure_capacity(NeuralAgent *agent) {
    int new_capacity;
    NeuralAgentTool *new_tools;
    int i;

    if (!agent) {
        return -1;
    }

    if (agent->capacity == 0) {
        new_capacity = 4;
    } else if (agent->count < agent->capacity) {
        return 0;
    } else {
        new_capacity = agent->capacity * 2;
    }

    new_tools = (NeuralAgentTool *)realloc(agent->tools, (size_t)new_capacity * sizeof(NeuralAgentTool));
    if (!new_tools) {
        return -1;
    }

    for (i = agent->capacity; i < new_capacity; ++i) {
        new_tools[i].name = NULL;
        new_tools[i].trigger_substring = NULL;
        new_tools[i].fn = NULL;
    }

    agent->tools = new_tools;
    agent->capacity = new_capacity;
    return 0;
}

void neural_agent_init(NeuralAgent *agent) {
    if (!agent) {
        return;
    }
    agent->tools = NULL;
    agent->count = 0;
    agent->capacity = 0;
}

void neural_agent_free(NeuralAgent *agent) {
    int i;
    if (!agent) {
        return;
    }

    for (i = 0; i < agent->count; ++i) {
        free(agent->tools[i].name);
        free(agent->tools[i].trigger_substring);
        agent->tools[i].name = NULL;
        agent->tools[i].trigger_substring = NULL;
        agent->tools[i].fn = NULL;
    }

    free(agent->tools);
    agent->tools = NULL;
    agent->count = 0;
    agent->capacity = 0;
}

int neural_agent_register_tool(NeuralAgent *agent,
                                const char *name,
                                const char *trigger_substring,
                                NeuralToolFn fn) {
    int i;

    if (!agent || !name || !fn) {
        return -1;
    }

    /* Overwrite if already registered by name. */
    for (i = 0; i < agent->count; ++i) {
        if (agent->tools[i].name && strcmp(agent->tools[i].name, name) == 0) {
            free(agent->tools[i].trigger_substring);
            agent->tools[i].trigger_substring = dup_string(trigger_substring);
            if (trigger_substring && !agent->tools[i].trigger_substring) return -1;
            agent->tools[i].fn = fn;
            return 0;
        }
    }

    if (ensure_capacity(agent) != 0) {
        return -1;
    }

    agent->tools[agent->count].name = dup_string(name);
    agent->tools[agent->count].trigger_substring = dup_string(trigger_substring);
    agent->tools[agent->count].fn = fn;

    if (!agent->tools[agent->count].name || (trigger_substring && !agent->tools[agent->count].trigger_substring)) {
        free(agent->tools[agent->count].name);
        free(agent->tools[agent->count].trigger_substring);
        agent->tools[agent->count].name = NULL;
        agent->tools[agent->count].trigger_substring = NULL;
        agent->tools[agent->count].fn = NULL;
        return -1;
    }

    agent->count += 1;
    return 0;
}

const char *neural_agent_call_tool(const NeuralAgent *agent, const char *input) {
    int i;

    if (!agent || !input) {
        return NULL;
    }

    for (i = 0; i < agent->count; ++i) {
        const NeuralAgentTool *tool = &agent->tools[i];
        if (!tool->fn || !tool->name) {
            continue;
        }

        if (!tool->trigger_substring) {
            continue;
        }

        if (strstr(input, tool->trigger_substring) != NULL) {
            return tool->fn(input);
        }
    }

    return NULL;
}

