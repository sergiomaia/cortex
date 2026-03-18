#include "action_controller.h"
#include "action_response.h"

void action_controller_render_json(ActionResponse *res, int status, const char *json_body) {
    action_response_set(res, status, json_body);
}

void action_controller_render_text(ActionResponse *res, int status, const char *text_body) {
    action_response_set(res, status, text_body);
}

