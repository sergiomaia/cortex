#ifndef PULSE_H
#define PULSE_H

#include <stdarg.h>
#include <stdio.h>
#include <time.h>

/*
 * Pulse — Cortex structured logging subsystem.
 *
 * Pulse is the canonical observability surface for Cortex: it provides a
 * Rails-like developer experience in TEXT mode while emitting valid NDJSON
 * (one JSON object per line) for production aggregators. Pulse has no
 * external dependencies, performs no heap allocation on the hot path, and is
 * safe to call from multiple threads.
 *
 * Calling pulse_log* before pulse_init is intentionally a silent no-op so
 * libraries can instrument themselves without forcing a logger lifecycle on
 * their consumers. Once pulse_init runs, every emit is mutex-serialized and
 * flushed to its destination immediately.
 */

#ifdef __cplusplus
extern "C" {
#endif

/* ─────────────────────────────────────────────
   Log levels (ordered ascending by severity).
   PULSE_SILENT raises the bar above PULSE_FATAL
   to suppress every emit. FATAL still aborts().
   ───────────────────────────────────────────── */
typedef enum {
    PULSE_DEBUG = 0,
    PULSE_INFO,
    PULSE_WARN,
    PULSE_ERROR,
    PULSE_FATAL,
    PULSE_SILENT
} PulseLevel;

/* ─────────────────────────────────────────────
   Output formats.
   ───────────────────────────────────────────── */
typedef enum {
    PULSE_FMT_TEXT,
    PULSE_FMT_JSON,
} PulseFormat;

/* ─────────────────────────────────────────────
   Logger configuration.

   When `log_file` is non-NULL and non-empty, Pulse opens it for append and
   takes ownership of the resulting stream. Otherwise the caller-provided
   `output` stream is used (defaulting to stderr if both are NULL).

   `colorize` is honored only when the active stream is not a file: writing
   to a file always disables ANSI colors so log aggregators see clean text.

   `max_bytes` and `max_files` enable rotation when writing to an owned file.
   max_bytes <= 0 disables rotation. max_files <= 0 disables rotation. Stdin/
   stdout/stderr are never rotated.
   ───────────────────────────────────────────── */
typedef struct {
    PulseLevel  level;
    PulseFormat format;
    FILE       *output;
    int         colorize;

    const char *log_file;
    long        max_bytes;
    int         max_files;
} PulseConfig;

/* ─────────────────────────────────────────────
   Structured event fields.
   Keys and values are bounded, stack-allocated
   strings to keep emit() heap-free.
   ───────────────────────────────────────────── */
#define PULSE_FIELD_MAX   16
#define PULSE_KEY_MAX     32
#define PULSE_VAL_MAX    128

typedef struct {
    char key[PULSE_KEY_MAX];
    char val[PULSE_VAL_MAX];
} PulseField;

typedef struct {
    PulseLevel       level;
    const char      *module;
    const char      *message;
    struct timespec  ts;

    PulseField fields[PULSE_FIELD_MAX];
    int        nfields;
} PulseEvent;

/* ─────────────────────────────────────────────
   Lifecycle.
   pulse_init may be called multiple times: each
   call closes any previously owned stream before
   applying the new configuration. Passing NULL
   resets Pulse to safe defaults (TEXT to stderr,
   level INFO).
   ───────────────────────────────────────────── */
void pulse_init(const PulseConfig *cfg);
void pulse_shutdown(void);

/* ─────────────────────────────────────────────
   Low-level emit.

   The single centralized output path. Caller
   builds the PulseEvent (including timestamp);
   pulse_emit handles filtering, formatting,
   rotation, and abort() on FATAL.
   ───────────────────────────────────────────── */
void pulse_emit(const PulseEvent *ev);

/* ─────────────────────────────────────────────
   Standard logging — printf-style message.
   Safe to call before pulse_init (silent no-op,
   except FATAL which still aborts).
   ───────────────────────────────────────────── */
void pulse_log(PulseLevel level,
               const char *module,
               const char *fmt,
               ...)
#if defined(__GNUC__) || defined(__clang__)
    __attribute__((format(printf, 3, 4)))
#endif
;

/* ─────────────────────────────────────────────
   Structured logging — variadic key/value pairs.
   The argument list MUST end with a NULL sentinel:
       pulse_log_fields(PULSE_INFO, "action",
                        "request completed",
                        "method", "GET",
                        "path",   "/posts",
                        NULL);
   Both keys and values must be const char *. NULL
   values are coerced to the empty string.
   ───────────────────────────────────────────── */
void pulse_log_fields(PulseLevel level,
                      const char *module,
                      const char *message,
                      ...)
#if defined(__GNUC__) || defined(__clang__)
    __attribute__((sentinel))
#endif
;

/* ─────────────────────────────────────────────
   Environment bootstrap. Reads CORE_ENV,
   CORE_LOG_LEVEL and CORE_LOG_FILE and
   delegates to pulse_init.
   ───────────────────────────────────────────── */
void pulse_init_from_env(void);

/* ─────────────────────────────────────────────
   Convenience macros — printf-style.
   ───────────────────────────────────────────── */
#define LOG_DEBUG(mod, fmt, ...) \
    pulse_log(PULSE_DEBUG, mod, fmt, ##__VA_ARGS__)

#define LOG_INFO(mod, fmt, ...) \
    pulse_log(PULSE_INFO, mod, fmt, ##__VA_ARGS__)

#define LOG_WARN(mod, fmt, ...) \
    pulse_log(PULSE_WARN, mod, fmt, ##__VA_ARGS__)

#define LOG_ERROR(mod, fmt, ...) \
    pulse_log(PULSE_ERROR, mod, fmt, ##__VA_ARGS__)

#define LOG_FATAL(mod, fmt, ...) \
    pulse_log(PULSE_FATAL, mod, fmt, ##__VA_ARGS__)

#define LOG_WITH(level, mod, msg, ...) \
    pulse_log_fields(level, mod, msg, ##__VA_ARGS__, NULL)

#ifdef __cplusplus
}
#endif

#endif /* PULSE_H */
