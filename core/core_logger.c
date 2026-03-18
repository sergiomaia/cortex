#include "core_logger.h"

CoreLogger core_logger_init(FILE *stream, CoreLogLevel level) {
    CoreLogger logger;
    logger.stream = stream;
    logger.level = level;
    return logger;
}

void core_logger_log(CoreLogger *logger, CoreLogLevel level, const char *message) {
    const char *prefix;
    if (!logger || !logger->stream) {
        return;
    }

    if (level == CORE_LOG_LEVEL_ERROR) {
        prefix = "ERROR";
    } else {
        prefix = "INFO";
    }

    /* Very simple level filter: only log messages at or above logger->level. */
    if (level < logger->level) {
        return;
    }

    fprintf(logger->stream, "[%s] %s\n", prefix, message);
}

void core_logger_info(CoreLogger *logger, const char *message) {
    core_logger_log(logger, CORE_LOG_LEVEL_INFO, message);
}

void core_logger_error(CoreLogger *logger, const char *message) {
    core_logger_log(logger, CORE_LOG_LEVEL_ERROR, message);
}

