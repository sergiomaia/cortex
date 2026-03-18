#ifndef ACTION_DISPATCH_H
#define ACTION_DISPATCH_H

#include "action_router.h"

int action_dispatch(ActionRouter *router, ActionRequest *req, ActionResponse *res);

#endif /* ACTION_DISPATCH_H */

