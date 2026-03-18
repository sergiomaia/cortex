#ifndef ACTION_MIDDLEWARE_H
#define ACTION_MIDDLEWARE_H

#include "action_controller.h"
#include "action_request.h"
#include "action_response.h"

typedef void (*ActionMiddlewareNext)(void);

typedef void (*ActionMiddleware)(ActionRequest *req, ActionResponse *res, ActionMiddlewareNext next);

void action_middleware_clear(void);
int action_middleware_add(ActionMiddleware mw);
void action_middleware_dispatch(ActionHandler handler, ActionRequest *req, ActionResponse *res);

#endif /* ACTION_MIDDLEWARE_H */

