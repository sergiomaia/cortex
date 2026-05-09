#include "cortex_error.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "pulse.h"

static _Thread_local CortexError _tl_error;

static void cortex_error_truncate_str(char *dst, size_t dst_cap, const char *src)
{
    size_t n;

    if (!dst || dst_cap == 0) {
        return;
    }
    if (!src) {
        dst[0] = '\0';
        return;
    }
    n = strlen(src);
    if (n >= dst_cap) {
        n = dst_cap - 1;
    }
    memcpy(dst, src, n);
    dst[n] = '\0';
}

CortexError *cortex_last_error(void)
{
    return &_tl_error;
}

void cortex_clear_error(void)
{
    memset(&_tl_error, 0, sizeof(_tl_error));
    _tl_error.code = CORTEX_ERR_NONE;
}

int cortex_has_error(void)
{
    return _tl_error.code != CORTEX_ERR_NONE ? 1 : 0;
}

void cortex_error_set_simple(CortexErrCode code,
                             const char *source,
                             const char *file,
                             int line,
                             int errno_value,
                             const char *msg)
{
    CortexError *e = &_tl_error;

    e->code = code;
    cortex_error_truncate_str(e->source, sizeof(e->source), source);
    cortex_error_truncate_str(e->message, sizeof(e->message), msg ? msg : "");
    e->file = file;
    e->line = line;
    e->sys_errno = errno_value;
}

void cortex_error_set_formatted(CortexErrCode code,
                                const char *source,
                                const char *file,
                                int line,
                                int errno_value,
                                const char *fmt,
                                ...)
{
    va_list ap;
    int n;
    CortexError *e = &_tl_error;

    e->code = code;
    cortex_error_truncate_str(e->source, sizeof(e->source), source);
    e->file = file;
    e->line = line;
    e->sys_errno = errno_value;

    va_start(ap, fmt);
    n = vsnprintf(e->message, sizeof(e->message), fmt, ap);
    va_end(ap);
    if (n < 0) {
        e->message[0] = '\0';
    }
    if ((size_t)n >= sizeof(e->message)) {
        e->message[sizeof(e->message) - 1] = '\0';
    }
}

int cortex_err_to_http_status(CortexErrCode code)
{
    switch (code) {
    case CORTEX_ERR_ACTION_BAD_REQUEST:
        return 400;
    case CORTEX_ERR_ACTION_UNAUTHORIZED:
    case CORTEX_ERR_GUARD_AUTH_FAILED:
    case CORTEX_ERR_GUARD_TOKEN_EXPIRED:
    case CORTEX_ERR_GUARD_TOKEN_INVALID:
        return 401;
    case CORTEX_ERR_ACTION_FORBIDDEN:
        return 403;
    case CORTEX_ERR_ACTION_NOT_FOUND:
    case CORTEX_ERR_ACTIVE_NOT_FOUND:
        return 404;
    case CORTEX_ERR_ACTIVE_VALIDATION:
        return 422;
    default:
        return 500;
    }
}

const char *cortex_err_code_str(CortexErrCode code)
{
    switch (code) {
    case CORTEX_ERR_NONE:
        return "NONE";
    case CORTEX_ERR_CORE_CONFIG_NOT_FOUND:
        return "CORE_CONFIG_NOT_FOUND";
    case CORTEX_ERR_CORE_CONFIG_PARSE:
        return "CORE_CONFIG_PARSE";
    case CORTEX_ERR_CORE_OOM:
        return "CORE_OOM";
    case CORTEX_ERR_DB_CONNECT:
        return "DB_CONNECT";
    case CORTEX_ERR_DB_EXEC:
        return "DB_EXEC";
    case CORTEX_ERR_DB_PREPARE:
        return "DB_PREPARE";
    case CORTEX_ERR_DB_BUSY:
        return "DB_BUSY";
    case CORTEX_ERR_DB_CONSTRAINT:
        return "DB_CONSTRAINT";
    case CORTEX_ERR_DB_MIGRATION_PENDING:
        return "DB_MIGRATION_PENDING";
    case CORTEX_ERR_ACTIVE_VALIDATION:
        return "ACTIVE_VALIDATION";
    case CORTEX_ERR_ACTIVE_NOT_FOUND:
        return "ACTIVE_NOT_FOUND";
    case CORTEX_ERR_ACTIVE_READONLY:
        return "ACTIVE_READONLY";
    case CORTEX_ERR_ACTION_NOT_FOUND:
        return "ACTION_NOT_FOUND";
    case CORTEX_ERR_ACTION_UNAUTHORIZED:
        return "ACTION_UNAUTHORIZED";
    case CORTEX_ERR_ACTION_FORBIDDEN:
        return "ACTION_FORBIDDEN";
    case CORTEX_ERR_ACTION_BAD_REQUEST:
        return "ACTION_BAD_REQUEST";
    case CORTEX_ERR_ACTION_INTERNAL:
        return "ACTION_INTERNAL";
    case CORTEX_ERR_GUARD_AUTH_FAILED:
        return "GUARD_AUTH_FAILED";
    case CORTEX_ERR_GUARD_TOKEN_EXPIRED:
        return "GUARD_TOKEN_EXPIRED";
    case CORTEX_ERR_GUARD_TOKEN_INVALID:
        return "GUARD_TOKEN_INVALID";
    case CORTEX_ERR_FLOW_QUEUE_FULL:
        return "FLOW_QUEUE_FULL";
    case CORTEX_ERR_FLOW_JOB_FAILED:
        return "FLOW_JOB_FAILED";
    case CORTEX_ERR_FLOW_TIMEOUT:
        return "FLOW_TIMEOUT";
    case CORTEX_ERR_NEURAL_LLM_UNAVAILABLE:
        return "NEURAL_LLM_UNAVAILABLE";
    case CORTEX_ERR_NEURAL_CONTEXT_TOO_LARGE:
        return "NEURAL_CONTEXT_TOO_LARGE";
    case CORTEX_ERR_NEURAL_PARSE:
        return "NEURAL_PARSE";
    case CORTEX_ERR_CACHE_MISS:
        return "CACHE_MISS";
    case CORTEX_ERR_CACHE_BACKEND:
        return "CACHE_BACKEND";
    case CORTEX_ERR_IO:
        return "IO";
    case CORTEX_ERR_TIMEOUT:
        return "TIMEOUT";
    case CORTEX_ERR_INVALID_ARGUMENT:
        return "INVALID_ARGUMENT";
    case CORTEX_ERR_NOT_IMPLEMENTED:
        return "NOT_IMPLEMENTED";
    case CORTEX_ERR_UNKNOWN:
    default:
        return "UNKNOWN";
    }
}

void cortex_err_print(const CortexError *err)
{
    char at_buf[CORTEX_ERR_SRC_MAX + 32];
    char errno_buf[16];
    const char *msg;
    const char *source;

    if (!err || err->code == CORTEX_ERR_NONE) {
        return;
    }

    msg    = err->message[0] ? err->message : "(no message)";
    source = err->source[0]  ? err->source  : "?";

    snprintf(at_buf,    sizeof(at_buf),    "%s:%d", err->file ? err->file : "?", err->line);
    snprintf(errno_buf, sizeof(errno_buf), "%d",    err->sys_errno);

    /*
     * Convert the raw CortexError into a structured Pulse event. Routing all
     * framework errors through Pulse keeps observability backends consistent
     * (NDJSON in production, Rails-like text in development).
     */
    pulse_log_fields(PULSE_ERROR,
                     "cortex.error",
                     msg,
                     "code",   cortex_err_code_str(err->code),
                     "source", source,
                     "at",     at_buf,
                     "errno",  errno_buf,
                     NULL);
}
