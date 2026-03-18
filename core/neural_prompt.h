#ifndef NEURAL_PROMPT_H
#define NEURAL_PROMPT_H

typedef struct {
    const char *key;   /* Without braces, e.g. "name" */
    const char *value; /* May be NULL -> treated as empty string */
} NeuralPromptVar;

/* Renders `template_str` by replacing occurrences of {{key}} with the
 * matching value from `vars`.
 *
 * Missing variables are replaced with an empty string.
 *
 * The result is written into `out` (always NUL-terminated on success).
 * Returns 0 on success, -1 on invalid inputs / insufficient buffer. */
int neural_prompt_render(const char *template_str,
                          const NeuralPromptVar *vars,
                          int var_count,
                          char *out,
                          int out_size);

#endif /* NEURAL_PROMPT_H */

