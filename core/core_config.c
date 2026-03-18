#include "core_config.h"

CoreConfig core_config_default(void) {
    CoreConfig cfg;
    cfg.environment = "development";
    cfg.port = 3000;
    return cfg;
}

