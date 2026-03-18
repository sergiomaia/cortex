#ifndef PULSE_LOGGER_H
#define PULSE_LOGGER_H

typedef enum {
    PULSE_LOGGER_LEVEL_INFO = 0,
    PULSE_LOGGER_LEVEL_ERROR = 1
} PulseLoggerLevel;

/* Minimal logger that prints:
 * - INFO  -> stdout
 * - ERROR -> stderr
 */
void pulse_logger_log(PulseLoggerLevel level, const char *message);
void pulse_logger_info(const char *message);
void pulse_logger_error(const char *message);

#endif /* PULSE_LOGGER_H */

