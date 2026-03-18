#ifndef CORE_LOGGER_H
#define CORE_LOGGER_H

#include <stdio.h>

typedef enum {
    CORE_LOG_LEVEL_INFO,
    CORE_LOG_LEVEL_ERROR
} CoreLogLevel;

typedef struct {
    FILE *stream;
    CoreLogLevel level;
} CoreLogger;

CoreLogger core_logger_init(FILE *stream, CoreLogLevel level);

void core_logger_log(CoreLogger *logger, CoreLogLevel level, const char *message);

/* Simple convenience API */
void core_logger_info(CoreLogger *logger, const char *message);
void core_logger_error(CoreLogger *logger, const char *message);

#endif /* CORE_LOGGER_H */

