#include "core_env.h"

CoreEnv core_env_current(void) {
    CoreEnv env;
    env.name = "development";
    return env;
}

