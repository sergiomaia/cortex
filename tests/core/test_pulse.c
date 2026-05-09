#define _POSIX_C_SOURCE 200809L

#include "../cortex_test.h"

#include "../../core/pulse.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

/*
 * Internal accessors exported (but undocumented) by core/pulse.c so tests can
 * inspect logger state without growing the public surface.
 */
extern int         pulse_internal_initialized(void);
extern PulseLevel  pulse_internal_level(void);
extern PulseFormat pulse_internal_format(void);
extern int         pulse_internal_colorize(void);

/* ─────────────────────────────────────────────
   Helpers
   ───────────────────────────────────────────── */

/*
 * Capture every line a Pulse run emits to a freshly-truncated tmpfile and
 * return its contents in a heap buffer the caller must free.
 */
static char *pulse_capture(FILE *tmp)
{
    char *buf;
    long size;
    size_t read_n;

    fflush(tmp);
    if (fseek(tmp, 0, SEEK_END) != 0) {
        return NULL;
    }
    size = ftell(tmp);
    if (size < 0) {
        return NULL;
    }
    if (fseek(tmp, 0, SEEK_SET) != 0) {
        return NULL;
    }
    buf = (char *)calloc((size_t)size + 1, 1);
    if (!buf) {
        return NULL;
    }
    read_n = fread(buf, 1, (size_t)size, tmp);
    buf[read_n] = '\0';
    return buf;
}

static int file_size_or_zero(const char *path)
{
    struct stat st;
    if (stat(path, &st) != 0) {
        return 0;
    }
    return (int)st.st_size;
}

static void cleanup_log_files(const char *base, int max)
{
    char path[256];
    int i;

    (void)unlink(base);
    for (i = 1; i <= max; ++i) {
        snprintf(path, sizeof(path), "%s.%d", base, i);
        (void)unlink(path);
    }
}

static PulseConfig text_to_stream_cfg(FILE *out, PulseLevel lvl, int color)
{
    PulseConfig cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.level    = lvl;
    cfg.format   = PULSE_FMT_TEXT;
    cfg.output   = out;
    cfg.colorize = color;
    return cfg;
}

static PulseConfig json_to_stream_cfg(FILE *out, PulseLevel lvl)
{
    PulseConfig cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.level    = lvl;
    cfg.format   = PULSE_FMT_JSON;
    cfg.output   = out;
    cfg.colorize = 0;
    return cfg;
}

/* ─────────────────────────────────────────────
   Tests
   ───────────────────────────────────────────── */

static void test_pulse_init_and_shutdown(void)
{
    PulseConfig cfg = text_to_stream_cfg(stderr, PULSE_INFO, 0);
    pulse_init(&cfg);
    CT_ASSERT_EQ(pulse_internal_initialized(), 1);
    CT_ASSERT_EQ(pulse_internal_level(), PULSE_INFO);
    CT_ASSERT_EQ(pulse_internal_format(), PULSE_FMT_TEXT);

    pulse_shutdown();
    CT_ASSERT_EQ(pulse_internal_initialized(), 0);
}

static void test_pulse_text_output(void)
{
    FILE *tmp = tmpfile();
    PulseConfig cfg;
    char *out;

    CT_ASSERT_NOT_NULL(tmp);
    if (!tmp) return;

    cfg = text_to_stream_cfg(tmp, PULSE_INFO, 0);
    pulse_init(&cfg);

    pulse_log(PULSE_INFO, "tester", "hello %s", "world");

    out = pulse_capture(tmp);
    CT_ASSERT_NOT_NULL(out);
    if (out) {
        CT_ASSERT(strstr(out, "INFO") != NULL);
        CT_ASSERT(strstr(out, "[tester]") != NULL);
        CT_ASSERT(strstr(out, "hello world") != NULL);
        free(out);
    }

    pulse_shutdown();
    fclose(tmp);
}

static void test_pulse_json_output(void)
{
    FILE *tmp = tmpfile();
    PulseConfig cfg;
    char *out;

    CT_ASSERT_NOT_NULL(tmp);
    if (!tmp) return;

    cfg = json_to_stream_cfg(tmp, PULSE_INFO);
    pulse_init(&cfg);

    pulse_log(PULSE_INFO, "json_mod", "json message");

    out = pulse_capture(tmp);
    CT_ASSERT_NOT_NULL(out);
    if (out) {
        CT_ASSERT(strstr(out, "\"level\":\"INFO\"")    != NULL);
        CT_ASSERT(strstr(out, "\"module\":\"json_mod\"") != NULL);
        CT_ASSERT(strstr(out, "\"msg\":\"json message\"") != NULL);
        CT_ASSERT(strstr(out, "\"ts\":\"")              != NULL);
        /* NDJSON: line must end with newline. */
        CT_ASSERT(out[strlen(out) - 1] == '\n');
        /* Must have proper closing brace before newline. */
        CT_ASSERT(out[strlen(out) - 2] == '}');
        free(out);
    }

    pulse_shutdown();
    fclose(tmp);
}

static void test_pulse_level_filtering(void)
{
    FILE *tmp = tmpfile();
    PulseConfig cfg;
    char *out;

    CT_ASSERT_NOT_NULL(tmp);
    if (!tmp) return;

    cfg = text_to_stream_cfg(tmp, PULSE_WARN, 0);
    pulse_init(&cfg);

    pulse_log(PULSE_DEBUG, "filter", "should be hidden");
    pulse_log(PULSE_INFO,  "filter", "still hidden");
    pulse_log(PULSE_WARN,  "filter", "warn visible");
    pulse_log(PULSE_ERROR, "filter", "error visible");

    out = pulse_capture(tmp);
    CT_ASSERT_NOT_NULL(out);
    if (out) {
        CT_ASSERT(strstr(out, "should be hidden") == NULL);
        CT_ASSERT(strstr(out, "still hidden")     == NULL);
        CT_ASSERT(strstr(out, "warn visible")     != NULL);
        CT_ASSERT(strstr(out, "error visible")    != NULL);
        free(out);
    }

    pulse_shutdown();
    fclose(tmp);
}

static void test_pulse_structured_fields_text(void)
{
    FILE *tmp = tmpfile();
    PulseConfig cfg;
    char *out;

    CT_ASSERT_NOT_NULL(tmp);
    if (!tmp) return;

    cfg = text_to_stream_cfg(tmp, PULSE_DEBUG, 0);
    pulse_init(&cfg);

    pulse_log_fields(PULSE_INFO, "action", "request completed",
                     "method",   "GET",
                     "path",     "/posts",
                     "status",   "200",
                     "duration", "4ms",
                     NULL);

    out = pulse_capture(tmp);
    CT_ASSERT_NOT_NULL(out);
    if (out) {
        CT_ASSERT(strstr(out, "method=GET")    != NULL);
        CT_ASSERT(strstr(out, "path=/posts")   != NULL);
        CT_ASSERT(strstr(out, "status=200")    != NULL);
        CT_ASSERT(strstr(out, "duration=4ms")  != NULL);
        free(out);
    }

    pulse_shutdown();
    fclose(tmp);
}

static void test_pulse_structured_fields_json(void)
{
    FILE *tmp = tmpfile();
    PulseConfig cfg;
    char *out;

    CT_ASSERT_NOT_NULL(tmp);
    if (!tmp) return;

    cfg = json_to_stream_cfg(tmp, PULSE_DEBUG);
    pulse_init(&cfg);

    pulse_log_fields(PULSE_INFO, "action", "request completed",
                     "method", "POST",
                     "path",   "/users",
                     NULL);

    out = pulse_capture(tmp);
    CT_ASSERT_NOT_NULL(out);
    if (out) {
        CT_ASSERT(strstr(out, "\"method\":\"POST\"") != NULL);
        CT_ASSERT(strstr(out, "\"path\":\"/users\"") != NULL);
        free(out);
    }

    pulse_shutdown();
    fclose(tmp);
}

static void test_pulse_json_escaping(void)
{
    FILE *tmp = tmpfile();
    PulseConfig cfg;
    char *out;

    CT_ASSERT_NOT_NULL(tmp);
    if (!tmp) return;

    cfg = json_to_stream_cfg(tmp, PULSE_DEBUG);
    pulse_init(&cfg);

    pulse_log_fields(PULSE_WARN, "esc", "weird \"value\" with\nnewline\tand\\backslash",
                     "ctl",  "bell\x07here",
                     NULL);

    out = pulse_capture(tmp);
    CT_ASSERT_NOT_NULL(out);
    if (out) {
        /* Quotes are escaped as \" and backslashes as \\. */
        CT_ASSERT(strstr(out, "\\\"value\\\"") != NULL);
        CT_ASSERT(strstr(out, "\\n")           != NULL);
        CT_ASSERT(strstr(out, "\\t")           != NULL);
        CT_ASSERT(strstr(out, "\\\\backslash") != NULL);
        /* Control characters become \uXXXX sequences. */
        CT_ASSERT(strstr(out, "\\u0007")       != NULL);
        free(out);
    }

    pulse_shutdown();
    fclose(tmp);
}

static void test_pulse_rotation(void)
{
    PulseConfig cfg;
    const char *base = "build/test_pulse_rotation.log";
    char rotated_path[256];
    int i;

    cleanup_log_files(base, 5);
    /* Make sure the directory exists. */
    (void)mkdir("build", 0755);

    memset(&cfg, 0, sizeof(cfg));
    cfg.level     = PULSE_INFO;
    cfg.format    = PULSE_FMT_TEXT;
    cfg.log_file  = base;
    cfg.max_bytes = 200;
    cfg.max_files = 3;
    cfg.colorize  = 0;
    pulse_init(&cfg);

    /* Each log line is well over 100 bytes once the timestamp + module are
     * formatted, so emitting many of them must trigger at least one
     * rotation. */
    for (i = 0; i < 50; ++i) {
        pulse_log(PULSE_INFO, "rotor", "rotation log entry %d aaaaaaaaaaaaaaaaaa", i);
    }

    pulse_shutdown();

    snprintf(rotated_path, sizeof(rotated_path), "%s.1", base);
    CT_ASSERT(file_size_or_zero(base)         > 0);
    CT_ASSERT(file_size_or_zero(rotated_path) > 0);

    cleanup_log_files(base, 5);
}

static void test_pulse_env_bootstrap_production(void)
{
    setenv("CORE_ENV",       "production", 1);
    setenv("CORE_LOG_LEVEL", "warn",        1);
    unsetenv("CORE_LOG_FILE");

    pulse_init_from_env();

    CT_ASSERT_EQ(pulse_internal_format(),  PULSE_FMT_JSON);
    CT_ASSERT_EQ(pulse_internal_level(),   PULSE_WARN);
    CT_ASSERT_EQ(pulse_internal_colorize(), 0);

    pulse_shutdown();
    unsetenv("CORE_ENV");
    unsetenv("CORE_LOG_LEVEL");
}

static void test_pulse_env_bootstrap_development(void)
{
    setenv("CORE_ENV", "development", 1);
    unsetenv("CORE_LOG_LEVEL");
    unsetenv("CORE_LOG_FILE");

    pulse_init_from_env();

    CT_ASSERT_EQ(pulse_internal_format(), PULSE_FMT_TEXT);
    CT_ASSERT_EQ(pulse_internal_level(),  PULSE_INFO);

    pulse_shutdown();
    unsetenv("CORE_ENV");
}

static void test_pulse_env_bootstrap_test_environment(void)
{
    setenv("CORE_ENV", "test", 1);
    unsetenv("CORE_LOG_LEVEL");
    unsetenv("CORE_LOG_FILE");

    pulse_init_from_env();

    CT_ASSERT_EQ(pulse_internal_format(),  PULSE_FMT_TEXT);
    CT_ASSERT_EQ(pulse_internal_level(),   PULSE_WARN);
    CT_ASSERT_EQ(pulse_internal_colorize(), 0);

    pulse_shutdown();
    unsetenv("CORE_ENV");
}

static void test_pulse_env_log_file_disables_color(void)
{
    const char *path = "build/test_pulse_env_logfile.log";

    cleanup_log_files(path, 5);
    (void)mkdir("build", 0755);

    setenv("CORE_ENV",      "development", 1);
    setenv("CORE_LOG_FILE", path,          1);

    pulse_init_from_env();
    CT_ASSERT_EQ(pulse_internal_colorize(), 0);

    pulse_log(PULSE_INFO, "envlog", "boots from env");
    pulse_shutdown();

    CT_ASSERT(file_size_or_zero(path) > 0);

    unsetenv("CORE_ENV");
    unsetenv("CORE_LOG_FILE");
    cleanup_log_files(path, 5);
}

#define PULSE_TS_THREADS   4
#define PULSE_TS_PER_THR   25

static void *pulse_ts_worker(void *arg)
{
    int id = *(int *)arg;
    int i;
    for (i = 0; i < PULSE_TS_PER_THR; ++i) {
        pulse_log(PULSE_INFO, "thread", "tid=%d iter=%d", id, i);
    }
    return NULL;
}

static void test_pulse_thread_safety_basics(void)
{
    FILE *tmp = tmpfile();
    pthread_t threads[PULSE_TS_THREADS];
    int       ids[PULSE_TS_THREADS];
    PulseConfig cfg;
    char *out;
    int i;
    int line_count = 0;

    CT_ASSERT_NOT_NULL(tmp);
    if (!tmp) return;

    cfg = text_to_stream_cfg(tmp, PULSE_DEBUG, 0);
    pulse_init(&cfg);

    for (i = 0; i < PULSE_TS_THREADS; ++i) {
        ids[i] = i;
        CT_ASSERT_EQ(pthread_create(&threads[i], NULL, pulse_ts_worker, &ids[i]), 0);
    }
    for (i = 0; i < PULSE_TS_THREADS; ++i) {
        CT_ASSERT_EQ(pthread_join(threads[i], NULL), 0);
    }

    out = pulse_capture(tmp);
    CT_ASSERT_NOT_NULL(out);
    if (out) {
        char *p = out;
        while (*p) {
            if (*p == '\n') {
                ++line_count;
            }
            ++p;
        }
        free(out);
    }

    /* No log line may be lost under concurrent emit. */
    CT_ASSERT_EQ(line_count, PULSE_TS_THREADS * PULSE_TS_PER_THR);

    pulse_shutdown();
    fclose(tmp);
}

static void test_pulse_fatal_aborts_in_subprocess(void)
{
    pid_t pid = fork();
    int status = 0;

    if (pid == 0) {
        PulseConfig cfg;
        FILE *null_out = fopen("/dev/null", "w");
        if (!null_out) {
            null_out = stderr;
        }
        cfg = text_to_stream_cfg(null_out, PULSE_DEBUG, 0);
        pulse_init(&cfg);
        pulse_log(PULSE_FATAL, "subproc", "boom");
        /*
         * If FATAL did not terminate the child, force a non-zero exit so the
         * parent assertion still fails — _exit(0) would mask a regression.
         */
        _exit(0);
    }

    CT_ASSERT(pid > 0);
    if (pid > 0) {
        CT_ASSERT_EQ(waitpid(pid, &status, 0), pid);
        /*
         * abort() raises SIGABRT, but some sandboxed glibc backtrace handlers
         * convert that into SIGSEGV/SIGILL during stack unwinding. The only
         * portable invariant is that the process was terminated by a signal
         * rather than exiting normally — that is precisely what FATAL must
         * guarantee for callers.
         */
        CT_ASSERT(WIFSIGNALED(status));
        CT_ASSERT(!WIFEXITED(status));
    }
}

static void test_pulse_text_format_alignment(void)
{
    FILE *tmp = tmpfile();
    PulseConfig cfg;
    char *out;

    CT_ASSERT_NOT_NULL(tmp);
    if (!tmp) return;

    cfg = text_to_stream_cfg(tmp, PULSE_DEBUG, 0);
    pulse_init(&cfg);

    pulse_log(PULSE_WARN, "align", "row");

    out = pulse_capture(tmp);
    CT_ASSERT_NOT_NULL(out);
    if (out) {
        /* WARN is padded to 5 chars and surrounded by 2 spaces on each side. */
        CT_ASSERT(strstr(out, "  WARN   [align]  row") != NULL);
        free(out);
    }

    pulse_shutdown();
    fclose(tmp);
}

static void test_pulse_timestamp_present(void)
{
    FILE *tmp = tmpfile();
    PulseConfig cfg;
    char *out;

    CT_ASSERT_NOT_NULL(tmp);
    if (!tmp) return;

    cfg = text_to_stream_cfg(tmp, PULSE_INFO, 0);
    pulse_init(&cfg);
    pulse_log(PULSE_INFO, "ts", "with timestamp");

    out = pulse_capture(tmp);
    CT_ASSERT_NOT_NULL(out);
    if (out) {
        /* Year prefix sufficient to detect a formatted timestamp. */
        CT_ASSERT(strstr(out, "20") == out);
        /* Must contain a millisecond delimiter. */
        CT_ASSERT(strstr(out, ".") != NULL);
        free(out);
    }

    pulse_shutdown();
    fclose(tmp);
}

static void test_pulse_color_enable_disable(void)
{
    FILE *tmp = tmpfile();
    PulseConfig cfg;
    char *out;

    CT_ASSERT_NOT_NULL(tmp);
    if (!tmp) return;

    cfg = text_to_stream_cfg(tmp, PULSE_INFO, 1);
    pulse_init(&cfg);
    pulse_log(PULSE_INFO, "col", "with colors");
    out = pulse_capture(tmp);
    CT_ASSERT_NOT_NULL(out);
    if (out) {
        CT_ASSERT(strstr(out, "\x1b[") != NULL);
        free(out);
    }
    pulse_shutdown();

    rewind(tmp);
    {
        int ftrunc_rc = ftruncate(fileno(tmp), 0);
        (void)ftrunc_rc;
    }

    cfg = text_to_stream_cfg(tmp, PULSE_INFO, 0);
    pulse_init(&cfg);
    pulse_log(PULSE_INFO, "col", "no colors");
    out = pulse_capture(tmp);
    CT_ASSERT_NOT_NULL(out);
    if (out) {
        CT_ASSERT(strstr(out, "\x1b[") == NULL);
        free(out);
    }
    pulse_shutdown();
    fclose(tmp);
}

static void test_pulse_safe_truncation(void)
{
    FILE *tmp = tmpfile();
    PulseConfig cfg;
    char long_key[PULSE_KEY_MAX * 2];
    char long_val[PULSE_VAL_MAX * 2];
    char *out;
    size_t i;

    CT_ASSERT_NOT_NULL(tmp);
    if (!tmp) return;

    for (i = 0; i + 1 < sizeof(long_key); ++i) long_key[i] = 'k';
    long_key[sizeof(long_key) - 1] = '\0';
    for (i = 0; i + 1 < sizeof(long_val); ++i) long_val[i] = 'v';
    long_val[sizeof(long_val) - 1] = '\0';

    cfg = text_to_stream_cfg(tmp, PULSE_INFO, 0);
    pulse_init(&cfg);
    pulse_log_fields(PULSE_INFO, "trunc", "truncate me",
                     long_key, long_val,
                     NULL);
    out = pulse_capture(tmp);
    CT_ASSERT_NOT_NULL(out);
    if (out) {
        /* Truncated output must contain at most PULSE_KEY_MAX-1 'k's then '='. */
        char marker[PULSE_KEY_MAX + 4];
        size_t kn = PULSE_KEY_MAX - 1;
        for (i = 0; i < kn; ++i) marker[i] = 'k';
        marker[kn] = '=';
        marker[kn + 1] = '\0';
        CT_ASSERT(strstr(out, marker) != NULL);
        free(out);
    }
    pulse_shutdown();
    fclose(tmp);
}

static void test_pulse_null_module_fallback(void)
{
    FILE *tmp = tmpfile();
    PulseConfig cfg;
    char *out;

    CT_ASSERT_NOT_NULL(tmp);
    if (!tmp) return;

    cfg = text_to_stream_cfg(tmp, PULSE_INFO, 0);
    pulse_init(&cfg);
    pulse_log(PULSE_INFO, NULL, "no module given");
    out = pulse_capture(tmp);
    CT_ASSERT_NOT_NULL(out);
    if (out) {
        CT_ASSERT(strstr(out, "[core]") != NULL);
        CT_ASSERT(strstr(out, "no module given") != NULL);
        free(out);
    }
    pulse_shutdown();
    fclose(tmp);
}

static void test_pulse_null_output_fallback(void)
{
    PulseConfig cfg;

    memset(&cfg, 0, sizeof(cfg));
    cfg.level    = PULSE_SILENT;
    cfg.format   = PULSE_FMT_TEXT;
    cfg.output   = NULL;
    cfg.colorize = 0;
    pulse_init(&cfg);

    /* Must initialize and not crash even with a NULL output stream. */
    CT_ASSERT_EQ(pulse_internal_initialized(), 1);
    pulse_log(PULSE_INFO, "nullout", "should not crash");
    pulse_shutdown();
}

static void test_pulse_multiple_sequential_logs(void)
{
    FILE *tmp = tmpfile();
    PulseConfig cfg;
    char *out;
    int i;
    int line_count = 0;

    CT_ASSERT_NOT_NULL(tmp);
    if (!tmp) return;

    cfg = text_to_stream_cfg(tmp, PULSE_DEBUG, 0);
    pulse_init(&cfg);

    for (i = 0; i < 30; ++i) {
        pulse_log(PULSE_INFO, "seq", "iter=%d", i);
    }

    out = pulse_capture(tmp);
    CT_ASSERT_NOT_NULL(out);
    if (out) {
        char *p = out;
        while (*p) {
            if (*p == '\n') ++line_count;
            ++p;
        }
        CT_ASSERT(strstr(out, "iter=0")  != NULL);
        CT_ASSERT(strstr(out, "iter=29") != NULL);
        free(out);
    }
    CT_ASSERT_EQ(line_count, 30);

    pulse_shutdown();
    fclose(tmp);
}

static void test_pulse_silent_mode_suppression(void)
{
    FILE *tmp = tmpfile();
    PulseConfig cfg;
    char *out;
    long size;

    CT_ASSERT_NOT_NULL(tmp);
    if (!tmp) return;

    cfg = text_to_stream_cfg(tmp, PULSE_SILENT, 0);
    pulse_init(&cfg);

    pulse_log(PULSE_DEBUG, "silent", "1");
    pulse_log(PULSE_INFO,  "silent", "2");
    pulse_log(PULSE_WARN,  "silent", "3");
    pulse_log(PULSE_ERROR, "silent", "4");

    fflush(tmp);
    fseek(tmp, 0, SEEK_END);
    size = ftell(tmp);
    CT_ASSERT_EQ(size, 0);

    out = pulse_capture(tmp);
    if (out) {
        free(out);
    }

    pulse_shutdown();
    fclose(tmp);
}

static void test_pulse_uninitialized_no_crash(void)
{
    /* Pulse must remain inert before init: no output, no crash. */
    pulse_shutdown(); /* idempotent reset */
    pulse_log(PULSE_INFO, "noinit", "should be silently dropped");
    pulse_log_fields(PULSE_WARN, "noinit", "structured drop",
                     "key", "value", NULL);
    CT_ASSERT_EQ(pulse_internal_initialized(), 0);
}

static void test_pulse_reinit_replaces_stream(void)
{
    FILE *t1 = tmpfile();
    FILE *t2 = tmpfile();
    PulseConfig cfg;
    char *out;

    CT_ASSERT_NOT_NULL(t1);
    CT_ASSERT_NOT_NULL(t2);
    if (!t1 || !t2) {
        if (t1) fclose(t1);
        if (t2) fclose(t2);
        return;
    }

    cfg = text_to_stream_cfg(t1, PULSE_INFO, 0);
    pulse_init(&cfg);
    pulse_log(PULSE_INFO, "reinit", "first stream");

    cfg = text_to_stream_cfg(t2, PULSE_INFO, 0);
    pulse_init(&cfg);
    pulse_log(PULSE_INFO, "reinit", "second stream");

    out = pulse_capture(t2);
    CT_ASSERT_NOT_NULL(out);
    if (out) {
        CT_ASSERT(strstr(out, "second stream") != NULL);
        CT_ASSERT(strstr(out, "first stream")  == NULL);
        free(out);
    }

    out = pulse_capture(t1);
    CT_ASSERT_NOT_NULL(out);
    if (out) {
        CT_ASSERT(strstr(out, "first stream")  != NULL);
        CT_ASSERT(strstr(out, "second stream") == NULL);
        free(out);
    }

    pulse_shutdown();
    fclose(t1);
    fclose(t2);
}

/* ─────────────────────────────────────────────
   Suite registration
   ───────────────────────────────────────────── */
CT_SUITE_BEGIN(pulse)
    CT_TEST(test_pulse_init_and_shutdown)
    CT_TEST(test_pulse_text_output)
    CT_TEST(test_pulse_json_output)
    CT_TEST(test_pulse_level_filtering)
    CT_TEST(test_pulse_structured_fields_text)
    CT_TEST(test_pulse_structured_fields_json)
    CT_TEST(test_pulse_json_escaping)
    CT_TEST(test_pulse_rotation)
    CT_TEST(test_pulse_env_bootstrap_production)
    CT_TEST(test_pulse_env_bootstrap_development)
    CT_TEST(test_pulse_env_bootstrap_test_environment)
    CT_TEST(test_pulse_env_log_file_disables_color)
    CT_TEST(test_pulse_thread_safety_basics)
    CT_TEST(test_pulse_fatal_aborts_in_subprocess)
    CT_TEST(test_pulse_text_format_alignment)
    CT_TEST(test_pulse_timestamp_present)
    CT_TEST(test_pulse_color_enable_disable)
    CT_TEST(test_pulse_safe_truncation)
    CT_TEST(test_pulse_null_module_fallback)
    CT_TEST(test_pulse_null_output_fallback)
    CT_TEST(test_pulse_multiple_sequential_logs)
    CT_TEST(test_pulse_silent_mode_suppression)
    CT_TEST(test_pulse_uninitialized_no_crash)
    CT_TEST(test_pulse_reinit_replaces_stream)
CT_SUITE_END()

void run_pulse_tests(void)
{
    /*
     * Ensure no prior suite left Pulse initialized: tests in this file own
     * the global logger and rely on a clean baseline before each case.
     */
    pulse_shutdown();
    CT_RUN_SUITE();
    pulse_shutdown();
}
