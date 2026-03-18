#include <ctype.h>
#include <string.h>

#include "neural_prompt.h"

static void trim_spaces(const char **start, const char **end) {
    const char *s = *start;
    const char *e = *end;

    while (s < e && isspace((unsigned char)*s)) {
        ++s;
    }
    while (e > s && isspace((unsigned char)*(e - 1))) {
        --e;
    }

    *start = s;
    *end = e;
}

static const char *lookup_var(const NeuralPromptVar *vars,
                                int var_count,
                                const char *key,
                                int key_len) {
    int i;
    if (!vars || !key || key_len <= 0) {
        return NULL;
    }
    for (i = 0; i < var_count; ++i) {
        if (!vars[i].key) {
            continue;
        }
        if ((int)strlen(vars[i].key) == key_len && strncmp(vars[i].key, key, key_len) == 0) {
            return vars[i].value;
        }
    }
    return NULL;
}

static int append_char(char *out, int out_size, int *pos, char c) {
    if (*pos + 1 >= out_size) {
        return -1;
    }
    out[*pos] = c;
    *pos += 1;
    return 0;
}

static int append_str(const char *s, char *out, int out_size, int *pos) {
    int n;
    if (!s) {
        s = "";
    }
    n = (int)strlen(s);
    if (*pos + n >= out_size) {
        return -1;
    }
    memcpy(out + *pos, s, n);
    *pos += n;
    return 0;
}

int neural_prompt_render(const char *template_str,
                          const NeuralPromptVar *vars,
                          int var_count,
                          char *out,
                          int out_size) {
    const char *p;
    int pos;

    if (!template_str || !out || out_size <= 0) {
        return -1;
    }

    if (out_size == 1) {
        out[0] = '\0';
        return -1;
    }

    pos = 0;
    out[0] = '\0';

    p = template_str;
    while (*p != '\0') {
        if (*p == '{' && *(p + 1) == '{') {
            const char *token_start;
            const char *token_end;
            const char *value;
            char token_buf[64];
            int token_len;

            p += 2; /* consume "{{" */
            token_start = p;

            /* Find "}}". */
            while (*p != '\0' && !(*p == '}' && *(p + 1) == '}')) {
                ++p;
            }
            if (*p == '\0') {
                /* No closing "}}": treat remainder literally. */
                if (append_char(out, out_size, &pos, '{') != 0) {
                    return -1;
                }
                if (append_char(out, out_size, &pos, '{') != 0) {
                    return -1;
                }
                return 0;
            }

            token_end = p;
            trim_spaces(&token_start, &token_end);
            token_len = (int)(token_end - token_start);

            if (token_len <= 0 || token_len >= (int)sizeof(token_buf)) {
                value = "";
            } else {
                memcpy(token_buf, token_start, (size_t)token_len);
                token_buf[token_len] = '\0';
                value = lookup_var(vars, var_count, token_buf, token_len);
            }

            if (append_str(value ? value : "", out, out_size, &pos) != 0) {
                return -1;
            }

            p += 2; /* consume "}}" */
            continue;
        }

        if (append_char(out, out_size, &pos, *p) != 0) {
            return -1;
        }
        ++p;
    }

    out[pos] = '\0';
    return 0;
}

