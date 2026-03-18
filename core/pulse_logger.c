#include <stdio.h>

#include "pulse_logger.h"

void pulse_logger_log(PulseLoggerLevel level, const char *message) {
    const char *prefix;
    FILE *out;

    if (!message) {
        return;
    }

    if (level == PULSE_LOGGER_LEVEL_ERROR) {
        prefix = "ERROR";
        out = stderr;
    } else {
        prefix = "INFO";
        out = stdout;
    }

    fprintf(out, "[%s] %s\n", prefix, message);
    fflush(out);
}

void pulse_logger_info(const char *message) {
    pulse_logger_log(PULSE_LOGGER_LEVEL_INFO, message);
}

void pulse_logger_error(const char *message) {
    pulse_logger_log(PULSE_LOGGER_LEVEL_ERROR, message);
}

