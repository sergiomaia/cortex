#include <stdlib.h>

#include "../../core/core_config.h"
#include "../test_assert.h"

void test_core_config_loads_environment_variables(void) {
    CoreConfig cfg;

    /* Set environment variables so that core_config_load picks them up. */
    setenv("CORE_ENV", "production", 1);
    setenv("CORE_PORT", "4242", 1);

    cfg = core_config_load();

    ASSERT_STR_EQ(cfg.environment, "production");
    ASSERT_EQ(cfg.port, 4242);

    /* Clean up so other tests are not affected. */
    unsetenv("CORE_ENV");
    unsetenv("CORE_PORT");
}

