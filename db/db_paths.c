#include "db_paths.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int db_path_for_environment(const char *environment, char *out, size_t out_size) {
    const char *env = environment;

    if (!out || out_size == 0) {
        return -1;
    }

    if (!env || env[0] == '\0') {
        env = getenv("CORE_ENV");
        if (!env || env[0] == '\0') {
            env = "development";
        }
    }

    if (snprintf(out, out_size, "db/%s.sqlite3", env) >= (int)out_size) {
        return -1;
    }
    return 0;
}
