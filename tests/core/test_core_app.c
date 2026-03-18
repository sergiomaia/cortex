#include <stdlib.h>

#include "../../core/core_app.h"
#include "../test_assert.h"

void test_core_app_init_does_not_crash(void) {
    CoreApp app = core_app_init();
    ASSERT_TRUE(app.initialized);
    core_app_shutdown(&app);
}

