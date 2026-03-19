/* Cortex app server entrypoint */
#include <stdio.h>
#include "action/action_dispatch.h"
#include "action/action_router.h"
#include "config/routes.h"

void app_register_routes(ActionRouter *router);

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    ActionRouter router;
    action_router_init(&router);
    app_register_routes(&router);
    printf("Listening on http://localhost:3000\n");
    return action_dispatch_serve_http(&router);
}
