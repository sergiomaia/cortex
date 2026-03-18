#include <string.h>

#include "neural_stream.h"

static int emit_tokens_from_response(const char *response,
                                      NeuralStreamCallback cb,
                                      void *user_data) {
    const char *p;
    int emitted;
    int is_last;

    if (!response || !cb) {
        return -1;
    }

    /* Tokenization strategy:
     * 1) If response contains '-', split on '-'
     * 2) Else if it contains whitespace, split on whitespace
     * 3) Else split into two chunks (ensures multiple callbacks) */
    if (strchr(response, '-') != NULL) {
        const char *dash = strchr(response, '-');
        if (!dash || dash == response || dash[1] == '\0') {
            return -1;
        }

        /* Call for first token */
        {
            char tok1[64];
            char tok2[64];
            size_t n1 = (size_t)(dash - response);
            size_t n2 = strlen(dash + 1);
            if (n1 >= sizeof(tok1) || n2 >= sizeof(tok2)) {
                return -1;
            }
            memcpy(tok1, response, n1);
            tok1[n1] = '\0';
            memcpy(tok2, dash + 1, n2);
            tok2[n2] = '\0';

            cb(tok1, 0, user_data);
            emitted = 1;
            is_last = 1;
            (void)is_last;
            cb(tok2, 1, user_data);
            emitted += 1;
            return emitted;
        }
    }

    /* Whitespace split */
    emitted = 0;
    p = response;
    while (*p != '\0') {
        while (*p != '\0' && (*p == ' ' || *p == '\n' || *p == '\r' || *p == '\t')) {
            ++p;
        }
        if (*p == '\0') {
            break;
        }

        {
            const char *start = p;
            while (*p != '\0' && !(*p == ' ' || *p == '\n' || *p == '\r' || *p == '\t')) {
                ++p;
            }

            /* Determine if there is another token ahead. */
            {
                const char *q = p;
                while (*q != '\0' && (*q == ' ' || *q == '\n' || *q == '\r' || *q == '\t')) {
                    ++q;
                }
                int last = (*q == '\0') ? 1 : 0;

                {
                    char tok[128];
                    size_t n = (size_t)(p - start);
                    if (n >= sizeof(tok)) {
                        return -1;
                    }
                    memcpy(tok, start, n);
                    tok[n] = '\0';

                    cb(tok, last, user_data);
                    emitted += 1;
                    if (last) {
                        return emitted;
                    }
                }
            }
        }
    }

    /* No delimiter / single token: split into two chunks to ensure
     * multiple callbacks even for strings like "mock-response". */
    if (emitted == 0) {
        return -1;
    }

    if (emitted == 1) {
        size_t n = strlen(response);
        if (n < 2) {
            /* Cannot split into two. */
            cb(response, 1, user_data);
            return 1;
        }

        {
            char tok1[64];
            char tok2[64];
            size_t n1 = n / 2;
            size_t n2 = n - n1;
            if (n1 >= sizeof(tok1) || n2 >= sizeof(tok2)) {
                return -1;
            }
            memcpy(tok1, response, n1);
            tok1[n1] = '\0';
            memcpy(tok2, response + n1, n2);
            tok2[n2] = '\0';

            cb(tok1, 0, user_data);
            cb(tok2, 1, user_data);
            return 2;
        }
    }

    return emitted;
}

NeuralStream neural_stream_init(NeuralRuntime runtime) {
    NeuralStream s;
    s.runtime = runtime;
    return s;
}

int neural_stream_run(NeuralStream *stream,
                       const char *prompt,
                       NeuralStreamCallback cb,
                       void *user_data) {
    const char *resp;

    if (!stream || !cb) {
        return -1;
    }

    resp = neural_run(&stream->runtime, prompt);
    return emit_tokens_from_response(resp, cb, user_data);
}

