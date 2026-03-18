#ifndef ACTION_CONTROLLER_H
#define ACTION_CONTROLLER_H

#include "action_request.h"
#include "action_response.h"

typedef void (*ActionHandler)(ActionRequest *req, ActionResponse *res);

typedef struct {
    const char *name;
} ActionController;

#endif /* ACTION_CONTROLLER_H */

