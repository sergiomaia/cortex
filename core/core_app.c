#include "core_app.h"

CoreApp core_app_init(void) {
    CoreApp app;
    app.initialized = 1;
    return app;
}

void core_app_shutdown(CoreApp *app) {
    if (!app) {
        return;
    }
    app->initialized = 0;
}

