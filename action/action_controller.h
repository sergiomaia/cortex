#ifndef ACTION_CONTROLLER_H
#define ACTION_CONTROLLER_H

#include "action_request.h"
#include "action_response.h"

typedef void (*ActionHandler)(ActionRequest *req, ActionResponse *res);

typedef struct {
    const char *name;
} ActionController;

/* Convenience helpers for controllers to render responses. */
void action_controller_render_json(ActionResponse *res, int status, const char *json_body);
void action_controller_render_text(ActionResponse *res, int status, const char *text_body);

#endif /* ACTION_CONTROLLER_H */

