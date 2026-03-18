#include "core_logger.h"

CoreLogger core_logger_init(FILE *stream) {
    CoreLogger logger;
    logger.stream = stream;
    return logger;
}

void core_logger_info(CoreLogger *logger, const char *message) {
    if (!logger || !logger->stream) {
        return;
    }
    fprintf(logger->stream, "[INFO] %s\n", message);
}

void core_logger_error(CoreLogger *logger, const char *message) {
    if (!logger || !logger->stream) {
        return;
    }
    fprintf(logger->stream, "[ERROR] %s\n", message);
}

