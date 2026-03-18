#include <stdlib.h>

#include "core_config.h"

CoreConfig core_config_load(void) {
    CoreConfig cfg;
    const char *env;

    env = getenv("CORE_ENV");
    if (!env || env[0] == '\0') {
        cfg.environment = "development";
    } else {
        cfg.environment = env;
    }

    env = getenv("CORE_PORT");
    if (!env || env[0] == '\0') {
        cfg.port = 3000;
    } else {
        int port = atoi(env);
        if (port <= 0) {
            cfg.port = 3000;
        } else {
            cfg.port = port;
        }
    }

    return cfg;
}

