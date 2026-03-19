#ifndef CONFIG_ROUTES_H
#define CONFIG_ROUTES_H

#include "../action/action_router.h"

/* Register all framework routes on the given router.
 *
 * This always includes:
 *   - GET "/"       → HTML welcome page
 *   - GET "/health" → simple health check
 *
 * Applications can extend this by adding their own routes directly
 * to the router before starting the HTTP server.
 */
void register_routes(ActionRouter *router);

#endif /* CONFIG_ROUTES_H */

