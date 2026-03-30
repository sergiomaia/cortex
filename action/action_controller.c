#include "action_controller.h"
#include "action_response.h"

#include <stdio.h>
#include <string.h>

void action_controller_render_json(ActionResponse *res, int status, const char *json_body) {
    action_response_set(res, status, json_body);
}

void action_controller_render_text(ActionResponse *res, int status, const char *text_body) {
    action_response_set(res, status, text_body);
}

void action_controller_render_redirect(ActionResponse *res, int status, const char *location) {
    static char buf[512];

    if (!res || !location || !location[0]) {
        return;
    }
    (void)snprintf(buf, sizeof(buf), "%s", location);
    res->status = status;
    res->body = "";
    res->content_type = "text/html";
    res->location = buf;
}

