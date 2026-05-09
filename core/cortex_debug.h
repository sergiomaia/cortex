#ifndef CORTEX_DEBUG_H
#define CORTEX_DEBUG_H

/*
 * Lightweight stack traces in development builds.
 * Disabled by default: define CORTEX_DEVELOPMENT to enable (e.g. -DCORTEX_DEVELOPMENT).
 */

#if defined(CORTEX_DEVELOPMENT)

#include <errno.h>
#include <execinfo.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static inline void cortex_debug_trace_here(void)
{
    void *frames[48];
    int depth;
    char **symbols;
    int i;

    depth = backtrace(frames, (int)(sizeof(frames) / sizeof(frames[0])));
    if (depth <= 0) {
        return;
    }

    symbols = backtrace_symbols(frames, depth);
    if (!symbols) {
        return;
    }

    fputs("[cortex:trace]\n", stderr);
    for (i = 0; i < depth; ++i) {
        if (symbols[i]) {
            fputs("  ", stderr);
            fputs(symbols[i], stderr);
            fputc('\n', stderr);
        }
    }
    fflush(stderr);

    /*
     * backtrace_symbols() allocates one contiguous block returned as char**;
     * a single free() is required — see execinfo(3).
     */
    free(symbols);
}

#define CORTEX_TRACE()                                                                               \
    do {                                                                                             \
        cortex_debug_trace_here();                                                                   \
    } while (0)

#else

#define CORTEX_TRACE() ((void)0)

#endif /* defined(CORTEX_DEVELOPMENT) */

#endif /* CORTEX_DEBUG_H */
