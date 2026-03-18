#include "action_response.h"

void action_response_set(ActionResponse *res, int status, const char *body) {
    if (!res) {
        return;
    }
    res->status = status;
    res->body = body;
}

