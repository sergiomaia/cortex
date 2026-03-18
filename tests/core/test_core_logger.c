#include <stdio.h>
#include <string.h>

#include "../../core/core_logger.h"
#include "../test_assert.h"

void test_core_logger_outputs_messages(void) {
    char buffer[512];
    size_t n;

    FILE *tmp = tmpfile();
    ASSERT_TRUE(tmp != NULL);

    CoreLogger logger = core_logger_init(tmp, CORE_LOG_LEVEL_INFO);

    core_logger_info(&logger, "info message");
    core_logger_error(&logger, "error message");

    fflush(tmp);
    rewind(tmp);

    n = fread(buffer, 1, sizeof(buffer) - 1, tmp);
    buffer[n] = '\0';

    ASSERT_TRUE(strstr(buffer, "info message") != NULL);
    ASSERT_TRUE(strstr(buffer, "error message") != NULL);

    fclose(tmp);
}

