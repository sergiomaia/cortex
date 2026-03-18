#ifndef ACTION_DISPATCH_H
#define ACTION_DISPATCH_H

#include "action_router.h"

/* Dispatch a single already-built request through the router. */
int action_dispatch(ActionRouter *router, ActionRequest *req, ActionResponse *res);

/* Minimal synchronous HTTP server bound to port 3000. */
int action_dispatch_serve_http(ActionRouter *router);

#endif /* ACTION_DISPATCH_H */

