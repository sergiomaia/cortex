#include "action_response.h"

void action_response_set(ActionResponse *res, int status, const char *body) {
    if (!res) {
        return;
    }
    res->status = status;
    res->body = body;
    if (!res->content_type) {
        res->content_type = "text/plain";
    }
}

void action_response_set_content_type(ActionResponse *res, const char *content_type) {
    if (!res) {
        return;
    }
    res->content_type = content_type;
}

