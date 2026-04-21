/* Cortex app server entrypoint */
#include <stdio.h>
#include "action/action_dispatch.h"
#include "action/action_router.h"
#include "config/routes.h"
#include "db/db_bootstrap.h"

void app_register_routes(ActionRouter *router);

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    if (cortex_db_bootstrap() != 0) {
        fprintf(stderr, "database bootstrap failed (check migration status)\n");
        return 1;
    }
    ActionRouter router;
    action_router_init(&router);
    app_register_routes(&router);
    printf("Listening on http://localhost:3000\n");
    {
        int rc = action_dispatch_serve_http(&router);
        cortex_db_shutdown();
        return rc;
    }
}
