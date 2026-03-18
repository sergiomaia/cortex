#ifndef CORE_LOGGER_H
#define CORE_LOGGER_H

#include <stdio.h>

typedef struct {
    FILE *stream;
} CoreLogger;

CoreLogger core_logger_init(FILE *stream);
void core_logger_info(CoreLogger *logger, const char *message);
void core_logger_error(CoreLogger *logger, const char *message);

#endif /* CORE_LOGGER_H */

