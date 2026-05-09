#define _POSIX_C_SOURCE 200809L

#include "pulse.h"

#include <errno.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

/*
 * Implementation notes
 * --------------------
 * - All emit work runs under g_mutex. The lock is released before any abort()
 *   call so a FATAL log never leaves the mutex held in the (extremely
 *   unlikely) event a signal handler attempts to acquire it.
 * - The hot path uses only stack buffers; no malloc/realloc occurs while a
 *   log line is being formatted or written.
 * - When pulse_init has not been called the public functions silently
 *   discard input, except FATAL which still aborts() — this preserves the
 *   "logging never crashes the application except for FATAL" contract while
 *   keeping libraries silent until an application opts in.
 */

#define PULSE_LINE_MAX  4096
#define PULSE_PATH_MAX  1024

#define ANSI_RESET    "\x1b[0m"
#define ANSI_DIM      "\x1b[2m"
#define ANSI_CYAN     "\x1b[36m"
#define ANSI_GREEN    "\x1b[32m"
#define ANSI_YELLOW   "\x1b[33m"
#define ANSI_RED      "\x1b[31m"
#define ANSI_BOLDRED  "\x1b[1;31m"

/* ─────────────────────────────────────────────
   Global state. All access must hold g_mutex.
   ───────────────────────────────────────────── */
static pthread_mutex_t g_mutex        = PTHREAD_MUTEX_INITIALIZER;
static int             g_initialized  = 0;
static PulseLevel      g_level        = PULSE_INFO;
static PulseFormat     g_format       = PULSE_FMT_TEXT;
static FILE           *g_stream       = NULL;
static int             g_owns_stream  = 0;
static int             g_colorize     = 0;
static long            g_max_bytes    = 0;
static int             g_max_files    = 0;
static off_t           g_bytes_written = 0;
static char            g_log_path[PULSE_PATH_MAX];

/* ─────────────────────────────────────────────
   Static helpers
   ───────────────────────────────────────────── */
static const char *level_text(PulseLevel l)
{
    switch (l) {
    case PULSE_DEBUG:  return "DEBUG";
    case PULSE_INFO:   return "INFO";
    case PULSE_WARN:   return "WARN";
    case PULSE_ERROR:  return "ERROR";
    case PULSE_FATAL:  return "FATAL";
    case PULSE_SILENT: return "SILENT";
    default:           return "INFO";
    }
}

static const char *level_color(PulseLevel l)
{
    switch (l) {
    case PULSE_DEBUG:  return ANSI_CYAN;
    case PULSE_INFO:   return ANSI_GREEN;
    case PULSE_WARN:   return ANSI_YELLOW;
    case PULSE_ERROR:  return ANSI_RED;
    case PULSE_FATAL:  return ANSI_BOLDRED;
    default:           return ANSI_RESET;
    }
}

static int eq_ci(const char *a, const char *b)
{
    if (!a || !b) {
        return 0;
    }
    return strcasecmp(a, b) == 0;
}

static PulseLevel parse_level_name(const char *name, PulseLevel fallback)
{
    if (!name) {
        return fallback;
    }
    if (eq_ci(name, "debug")) {
        return PULSE_DEBUG;
    }
    if (eq_ci(name, "info")) {
        return PULSE_INFO;
    }
    if (eq_ci(name, "warn") || eq_ci(name, "warning")) {
        return PULSE_WARN;
    }
    if (eq_ci(name, "error")) {
        return PULSE_ERROR;
    }
    if (eq_ci(name, "fatal")) {
        return PULSE_FATAL;
    }
    if (eq_ci(name, "silent") || eq_ci(name, "off") || eq_ci(name, "none")) {
        return PULSE_SILENT;
    }
    return fallback;
}

/* Bounded copy that always null-terminates. */
static void safe_copy(char *dst, size_t cap, const char *src)
{
    size_t n;

    if (!dst || cap == 0) {
        return;
    }
    if (!src) {
        dst[0] = '\0';
        return;
    }
    n = strlen(src);
    if (n >= cap) {
        n = cap - 1;
    }
    memcpy(dst, src, n);
    dst[n] = '\0';
}

/*
 * JSON-escape src into dst, observing the ECMA-404 control-character rules.
 * The destination is always null-terminated. Returns the number of bytes
 * written (excluding the terminator).
 */
static size_t json_escape(const char *src, char *dst, size_t cap)
{
    size_t pos = 0;

    if (!dst || cap == 0) {
        return 0;
    }
    dst[0] = '\0';
    if (!src) {
        return 0;
    }

    while (*src != '\0' && pos + 1 < cap) {
        unsigned char c = (unsigned char)*src;
        const char *fixed = NULL;
        char unicode_buf[8];

        switch (c) {
        case '"':  fixed = "\\\""; break;
        case '\\': fixed = "\\\\"; break;
        case '\b': fixed = "\\b";  break;
        case '\f': fixed = "\\f";  break;
        case '\n': fixed = "\\n";  break;
        case '\r': fixed = "\\r";  break;
        case '\t': fixed = "\\t";  break;
        default:
            if (c < 0x20U) {
                int w = snprintf(unicode_buf, sizeof(unicode_buf), "\\u%04x", c);
                if (w > 0 && (size_t)w < sizeof(unicode_buf)) {
                    fixed = unicode_buf;
                }
            }
            break;
        }

        if (fixed) {
            size_t len = strlen(fixed);
            if (pos + len + 1 >= cap) {
                break;
            }
            memcpy(dst + pos, fixed, len);
            pos += len;
        } else {
            dst[pos++] = (char)c;
        }
        ++src;
    }

    dst[pos] = '\0';
    return pos;
}

/* ─────────────────────────────────────────────
   Formatting
   ───────────────────────────────────────────── */
static int format_text(const PulseEvent *ev, char *buf, size_t cap, int colorize)
{
    struct tm tmv;
    time_t    secs;
    long      ms;
    int       n = 0;
    int       w;
    int       i;
    const char *lvl = level_text(ev->level);
    const char *col = level_color(ev->level);
    const char *mod = ev->module ? ev->module : "core";
    const char *msg = ev->message ? ev->message : "";

    if (cap < 2) {
        return 0;
    }

    secs = (time_t)ev->ts.tv_sec;
    if (!localtime_r(&secs, &tmv)) {
        memset(&tmv, 0, sizeof(tmv));
    }
    ms = (long)(ev->ts.tv_nsec / 1000000L);
    if (ms < 0) {
        ms = 0;
    }
    if (ms > 999) {
        ms = 999;
    }

    if (colorize) {
        w = snprintf(buf + n, cap - (size_t)n,
                     ANSI_DIM "%04d-%02d-%02d %02d:%02d:%02d.%03ld" ANSI_RESET
                     "  %s%-5s" ANSI_RESET
                     "  " ANSI_DIM "[%s]" ANSI_RESET
                     "  %s",
                     tmv.tm_year + 1900, tmv.tm_mon + 1, tmv.tm_mday,
                     tmv.tm_hour, tmv.tm_min, tmv.tm_sec, ms,
                     col, lvl, mod, msg);
    } else {
        w = snprintf(buf + n, cap - (size_t)n,
                     "%04d-%02d-%02d %02d:%02d:%02d.%03ld  %-5s  [%s]  %s",
                     tmv.tm_year + 1900, tmv.tm_mon + 1, tmv.tm_mday,
                     tmv.tm_hour, tmv.tm_min, tmv.tm_sec, ms,
                     lvl, mod, msg);
    }
    if (w < 0) {
        return 0;
    }
    if ((size_t)w >= cap - (size_t)n) {
        n = (int)cap - 1;
    } else {
        n += w;
    }

    for (i = 0; i < ev->nfields && (size_t)n + 1 < cap; ++i) {
        w = snprintf(buf + n, cap - (size_t)n,
                     "  %s=%s",
                     ev->fields[i].key,
                     ev->fields[i].val);
        if (w < 0) {
            break;
        }
        if ((size_t)w >= cap - (size_t)n) {
            n = (int)cap - 1;
            break;
        }
        n += w;
    }

    if ((size_t)n + 1 < cap) {
        buf[n++] = '\n';
        buf[n] = '\0';
    } else {
        buf[cap - 1] = '\n';
        n = (int)cap - 1;
    }
    return n;
}

static int format_json(const PulseEvent *ev, char *buf, size_t cap)
{
    struct tm tmv;
    time_t    secs;
    long      ms;
    int       n = 0;
    int       w;
    int       i;
    const char *lvl = level_text(ev->level);
    const char *mod = ev->module ? ev->module : "core";
    const char *msg = ev->message ? ev->message : "";
    char esc_mod[PULSE_KEY_MAX * 6 + 4];
    char esc_msg[PULSE_LINE_MAX];
    char esc_key[PULSE_KEY_MAX * 6 + 4];
    char esc_val[PULSE_VAL_MAX * 6 + 4];

    if (cap < 4) {
        return 0;
    }

    secs = (time_t)ev->ts.tv_sec;
    if (!gmtime_r(&secs, &tmv)) {
        memset(&tmv, 0, sizeof(tmv));
    }
    ms = (long)(ev->ts.tv_nsec / 1000000L);
    if (ms < 0) {
        ms = 0;
    }
    if (ms > 999) {
        ms = 999;
    }

    json_escape(mod, esc_mod, sizeof(esc_mod));
    json_escape(msg, esc_msg, sizeof(esc_msg));

    w = snprintf(buf + n, cap - (size_t)n,
                 "{\"ts\":\"%04d-%02d-%02dT%02d:%02d:%02d.%03ldZ\","
                 "\"level\":\"%s\","
                 "\"module\":\"%s\","
                 "\"msg\":\"%s\"",
                 tmv.tm_year + 1900, tmv.tm_mon + 1, tmv.tm_mday,
                 tmv.tm_hour, tmv.tm_min, tmv.tm_sec, ms,
                 lvl, esc_mod, esc_msg);
    if (w < 0) {
        return 0;
    }
    if ((size_t)w >= cap - (size_t)n) {
        n = (int)cap - 1;
        buf[n] = '\0';
        return n;
    }
    n += w;

    for (i = 0; i < ev->nfields && (size_t)n + 2 < cap; ++i) {
        json_escape(ev->fields[i].key, esc_key, sizeof(esc_key));
        json_escape(ev->fields[i].val, esc_val, sizeof(esc_val));
        w = snprintf(buf + n, cap - (size_t)n,
                     ",\"%s\":\"%s\"",
                     esc_key, esc_val);
        if (w < 0) {
            break;
        }
        if ((size_t)w >= cap - (size_t)n) {
            n = (int)cap - 1;
            break;
        }
        n += w;
    }

    if ((size_t)n + 2 < cap) {
        buf[n++] = '}';
        buf[n++] = '\n';
        buf[n] = '\0';
    } else if (cap >= 2) {
        buf[cap - 2] = '}';
        buf[cap - 1] = '\n';
        n = (int)cap - 1;
    }
    return n;
}

/* ─────────────────────────────────────────────
   Rotation. MUST be called with g_mutex held
   AND only when writing to an owned regular file.
   ───────────────────────────────────────────── */
static int is_rotatable(void)
{
    if (!g_owns_stream) {
        return 0;
    }
    if (g_log_path[0] == '\0') {
        return 0;
    }
    if (g_max_bytes <= 0 || g_max_files <= 0) {
        return 0;
    }
    if (g_stream == stdout || g_stream == stderr) {
        return 0;
    }
    return 1;
}

static void rotate_locked(void)
{
    char from[PULSE_PATH_MAX + 32];
    char to[PULSE_PATH_MAX + 32];
    int  i;

    if (!is_rotatable()) {
        return;
    }

    if (g_stream) {
        fflush(g_stream);
        fclose(g_stream);
        g_stream = NULL;
    }

    /* Discard the oldest backup if it exists. */
    snprintf(to, sizeof(to), "%s.%d", g_log_path, g_max_files);
    (void)unlink(to);

    /* Shift each existing rotation slot down by one. */
    for (i = g_max_files - 1; i >= 1; --i) {
        snprintf(from, sizeof(from), "%s.%d", g_log_path, i);
        snprintf(to,   sizeof(to),   "%s.%d", g_log_path, i + 1);
        (void)rename(from, to);
    }

    /* Move the active log to the first backup slot, then reopen it fresh. */
    snprintf(to, sizeof(to), "%s.1", g_log_path);
    (void)rename(g_log_path, to);

    g_stream = fopen(g_log_path, "a");
    g_bytes_written = 0;
    if (!g_stream) {
        /* Fall back to stderr but mark it as not owned to avoid double close. */
        g_stream = stderr;
        g_owns_stream = 0;
    }
}

/* ─────────────────────────────────────────────
   Lifecycle
   ───────────────────────────────────────────── */
static void close_owned_stream_locked(void)
{
    if (g_owns_stream && g_stream && g_stream != stdout && g_stream != stderr) {
        fflush(g_stream);
        fclose(g_stream);
    }
    g_stream = NULL;
    g_owns_stream = 0;
    g_log_path[0] = '\0';
    g_bytes_written = 0;
}

void pulse_init(const PulseConfig *cfg)
{
    pthread_mutex_lock(&g_mutex);

    close_owned_stream_locked();

    if (!cfg) {
        g_level = PULSE_INFO;
        g_format = PULSE_FMT_TEXT;
        g_stream = stderr;
        g_owns_stream = 0;
        g_colorize = isatty(fileno(stderr)) ? 1 : 0;
        g_max_bytes = 0;
        g_max_files = 0;
    } else {
        g_level = cfg->level;
        g_format = cfg->format;
        g_max_bytes = cfg->max_bytes;
        g_max_files = cfg->max_files;
        g_colorize = cfg->colorize ? 1 : 0;

        if (cfg->log_file && cfg->log_file[0] != '\0') {
            safe_copy(g_log_path, sizeof(g_log_path), cfg->log_file);
            g_stream = fopen(g_log_path, "a");
            if (g_stream) {
                struct stat st;
                g_owns_stream = 1;
                if (stat(g_log_path, &st) == 0 && st.st_size > 0) {
                    g_bytes_written = st.st_size;
                }
            } else {
                /* fopen failed: fall back to stderr without owning it. */
                g_log_path[0] = '\0';
                g_stream = stderr;
                g_owns_stream = 0;
            }
        } else if (cfg->output) {
            g_stream = cfg->output;
            g_owns_stream = 0;
        } else {
            g_stream = stderr;
            g_owns_stream = 0;
        }

        /* Files never receive ANSI escapes. */
        if (g_owns_stream) {
            g_colorize = 0;
        }
    }

    g_initialized = 1;
    pthread_mutex_unlock(&g_mutex);
}

void pulse_shutdown(void)
{
    pthread_mutex_lock(&g_mutex);
    close_owned_stream_locked();
    g_initialized = 0;
    g_level = PULSE_INFO;
    g_format = PULSE_FMT_TEXT;
    g_colorize = 0;
    g_max_bytes = 0;
    g_max_files = 0;
    pthread_mutex_unlock(&g_mutex);
}

/* ─────────────────────────────────────────────
   Emit
   ───────────────────────────────────────────── */
void pulse_emit(const PulseEvent *ev)
{
    char  line[PULSE_LINE_MAX];
    int   n = 0;
    int   should_abort = 0;

    if (!ev) {
        return;
    }

    pthread_mutex_lock(&g_mutex);

    if (!g_initialized || !g_stream) {
        if (ev->level == PULSE_FATAL) {
            should_abort = 1;
        }
        pthread_mutex_unlock(&g_mutex);
        if (should_abort) {
            abort();
        }
        return;
    }

    if (g_level == PULSE_SILENT || ev->level < g_level) {
        if (ev->level == PULSE_FATAL) {
            /* Per the contract, FATAL always aborts even when filtered. */
            should_abort = 1;
        }
        pthread_mutex_unlock(&g_mutex);
        if (should_abort) {
            abort();
        }
        return;
    }

    if (g_format == PULSE_FMT_JSON) {
        n = format_json(ev, line, sizeof(line));
    } else {
        n = format_text(ev, line, sizeof(line), g_colorize);
    }

    if (n > 0) {
        size_t to_write = (size_t)n;

        if (is_rotatable() &&
            g_bytes_written + (off_t)to_write > (off_t)g_max_bytes) {
            rotate_locked();
        }

        if (g_stream) {
            size_t wrote = fwrite(line, 1, to_write, g_stream);
            if (wrote > 0) {
                g_bytes_written += (off_t)wrote;
            }
            fflush(g_stream);
        }
    }

    if (ev->level == PULSE_FATAL) {
        should_abort = 1;
    }
    pthread_mutex_unlock(&g_mutex);

    if (should_abort) {
        abort();
    }
}

/* ─────────────────────────────────────────────
   Public formatting helpers
   ───────────────────────────────────────────── */
void pulse_log(PulseLevel level, const char *module, const char *fmt, ...)
{
    PulseEvent ev;
    char       msg[PULSE_LINE_MAX];
    va_list    ap;
    int        n;

    memset(&ev, 0, sizeof(ev));
    ev.level   = level;
    ev.module  = module;
    ev.nfields = 0;
    clock_gettime(CLOCK_REALTIME, &ev.ts);

    if (fmt) {
        va_start(ap, fmt);
        n = vsnprintf(msg, sizeof(msg), fmt, ap);
        va_end(ap);
        if (n < 0) {
            msg[0] = '\0';
        } else if ((size_t)n >= sizeof(msg)) {
            msg[sizeof(msg) - 1] = '\0';
        }
    } else {
        msg[0] = '\0';
    }

    ev.message = msg;
    pulse_emit(&ev);
}

void pulse_log_fields(PulseLevel level,
                      const char *module,
                      const char *message,
                      ...)
{
    PulseEvent  ev;
    va_list     ap;
    const char *key;

    memset(&ev, 0, sizeof(ev));
    ev.level   = level;
    ev.module  = module;
    ev.message = message;
    ev.nfields = 0;
    clock_gettime(CLOCK_REALTIME, &ev.ts);

    va_start(ap, message);
    while ((key = va_arg(ap, const char *)) != NULL) {
        const char *val = va_arg(ap, const char *);
        if (ev.nfields >= PULSE_FIELD_MAX) {
            /*
             * Drain remaining args so callers cannot trigger UB by passing
             * more than PULSE_FIELD_MAX pairs. The break-after-drain pattern
             * preserves NULL termination of va_args.
             */
            continue;
        }
        safe_copy(ev.fields[ev.nfields].key,
                  sizeof(ev.fields[ev.nfields].key),
                  key);
        safe_copy(ev.fields[ev.nfields].val,
                  sizeof(ev.fields[ev.nfields].val),
                  val ? val : "");
        ev.nfields++;
    }
    va_end(ap);

    pulse_emit(&ev);
}

/* ─────────────────────────────────────────────
   Environment bootstrap
   ───────────────────────────────────────────── */
void pulse_init_from_env(void)
{
    PulseConfig cfg;
    const char *env       = getenv("CORE_ENV");
    const char *level_env = getenv("CORE_LOG_LEVEL");
    const char *log_file  = getenv("CORE_LOG_FILE");

    memset(&cfg, 0, sizeof(cfg));

    if (env && eq_ci(env, "production")) {
        cfg.level    = PULSE_INFO;
        cfg.format   = PULSE_FMT_JSON;
        cfg.output   = stdout;
        cfg.colorize = 0;
    } else if (env && eq_ci(env, "test")) {
        cfg.level    = PULSE_WARN;
        cfg.format   = PULSE_FMT_TEXT;
        cfg.output   = stderr;
        cfg.colorize = 0;
    } else {
        /* development is the safe default for unknown / unset CORE_ENV. */
        cfg.level    = PULSE_INFO;
        cfg.format   = PULSE_FMT_TEXT;
        cfg.output   = stderr;
        cfg.colorize = isatty(fileno(stderr)) ? 1 : 0;
    }

    if (level_env && level_env[0] != '\0') {
        cfg.level = parse_level_name(level_env, cfg.level);
    }

    if (log_file && log_file[0] != '\0') {
        cfg.log_file  = log_file;
        if (cfg.max_bytes <= 0) {
            cfg.max_bytes = 10L * 1024L * 1024L; /* 10 MiB per file */
        }
        if (cfg.max_files <= 0) {
            cfg.max_files = 5;
        }
    }

    pulse_init(&cfg);
}

/* ─────────────────────────────────────────────
   Internal accessors (not declared in pulse.h).
   These exist so test code can introspect state
   without exporting setters into the public API.
   They are intentionally lock-protected.
   ───────────────────────────────────────────── */
int pulse_internal_initialized(void)
{
    int v;
    pthread_mutex_lock(&g_mutex);
    v = g_initialized;
    pthread_mutex_unlock(&g_mutex);
    return v;
}

PulseLevel pulse_internal_level(void)
{
    PulseLevel v;
    pthread_mutex_lock(&g_mutex);
    v = g_level;
    pthread_mutex_unlock(&g_mutex);
    return v;
}

PulseFormat pulse_internal_format(void)
{
    PulseFormat v;
    pthread_mutex_lock(&g_mutex);
    v = g_format;
    pthread_mutex_unlock(&g_mutex);
    return v;
}

int pulse_internal_colorize(void)
{
    int v;
    pthread_mutex_lock(&g_mutex);
    v = g_colorize;
    pthread_mutex_unlock(&g_mutex);
    return v;
}
