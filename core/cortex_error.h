#ifndef CORTEX_ERROR_H
#define CORTEX_ERROR_H

#include <errno.h>

typedef enum {
    CORTEX_ERR_NONE = 0,

    /* Core */
    CORTEX_ERR_CORE_CONFIG_NOT_FOUND,
    CORTEX_ERR_CORE_CONFIG_PARSE,
    CORTEX_ERR_CORE_OOM,

    /* DB */
    CORTEX_ERR_DB_CONNECT,
    CORTEX_ERR_DB_EXEC,
    CORTEX_ERR_DB_PREPARE,
    CORTEX_ERR_DB_BUSY,
    CORTEX_ERR_DB_CONSTRAINT,
    CORTEX_ERR_DB_MIGRATION_PENDING,

    /* Active */
    CORTEX_ERR_ACTIVE_VALIDATION,
    CORTEX_ERR_ACTIVE_NOT_FOUND,
    CORTEX_ERR_ACTIVE_READONLY,

    /* Action */
    CORTEX_ERR_ACTION_NOT_FOUND,
    CORTEX_ERR_ACTION_UNAUTHORIZED,
    CORTEX_ERR_ACTION_FORBIDDEN,
    CORTEX_ERR_ACTION_BAD_REQUEST,
    CORTEX_ERR_ACTION_INTERNAL,

    /* Guard */
    CORTEX_ERR_GUARD_AUTH_FAILED,
    CORTEX_ERR_GUARD_TOKEN_EXPIRED,
    CORTEX_ERR_GUARD_TOKEN_INVALID,

    /* Flow */
    CORTEX_ERR_FLOW_QUEUE_FULL,
    CORTEX_ERR_FLOW_JOB_FAILED,
    CORTEX_ERR_FLOW_TIMEOUT,

    /* Neural */
    CORTEX_ERR_NEURAL_LLM_UNAVAILABLE,
    CORTEX_ERR_NEURAL_CONTEXT_TOO_LARGE,
    CORTEX_ERR_NEURAL_PARSE,

    /* Cache */
    CORTEX_ERR_CACHE_MISS,
    CORTEX_ERR_CACHE_BACKEND,

    /* Generic */
    CORTEX_ERR_IO,
    CORTEX_ERR_TIMEOUT,
    CORTEX_ERR_INVALID_ARGUMENT,
    CORTEX_ERR_NOT_IMPLEMENTED,
    CORTEX_ERR_UNKNOWN,
} CortexErrCode;

#define CORTEX_ERR_MSG_MAX 256
#define CORTEX_ERR_SRC_MAX 128

typedef struct {
    CortexErrCode code;
    char message[CORTEX_ERR_MSG_MAX];
    char source[CORTEX_ERR_SRC_MAX];
    const char *file;
    int line;
    int sys_errno;
} CortexError;

/*
 * Owning accessor for this thread's last error slot. Never returns NULL.
 * Thread-local storage: safe for concurrent HTTP workers.
 */
CortexError *cortex_last_error(void);
void cortex_clear_error(void);
int cortex_has_error(void);

void cortex_error_set_simple(CortexErrCode code,
                             const char *source,
                             const char *file,
                             int line,
                             int errno_value,
                             const char *msg);

void cortex_error_set_formatted(CortexErrCode code,
                                const char *source,
                                const char *file,
                                int line,
                                int errno_value,
                                const char *fmt,
                                ...)
#if defined(__GNUC__) || defined(__clang__)
    __attribute__((format(printf, 6, 7)))
#endif
;

#define CORTEX_SET_ERROR(code, source, msg)                                                        \
    do {                                                                                           \
        int _cortex_errno_snap = (int)(errno);                                                      \
        cortex_error_set_simple((code), (source), __FILE__, __LINE__, _cortex_errno_snap, (msg)); \
    } while (0)

/*
 * Accepts zero or more printf args after fmt. Requires a GNU-compatible
 * preprocessor for the empty-variadic-args case (##__VA_ARGS__).
 */
#define CORTEX_SET_ERRORF(code, source, fmt, ...)                                                                  \
    do {                                                                                                           \
        int _cortex_errno_snap = (int)(errno);                                                                     \
        cortex_error_set_formatted((code), (source), __FILE__, __LINE__, _cortex_errno_snap, (fmt), ##__VA_ARGS__); \
    } while (0)

const char *cortex_err_code_str(CortexErrCode code);
void cortex_err_print(const CortexError *err);
int cortex_err_to_http_status(CortexErrCode code);

#endif /* CORTEX_ERROR_H */
