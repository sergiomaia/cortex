#ifndef ACTION_RESPONSE_H
#define ACTION_RESPONSE_H

typedef struct {
    int status;
    const char *body;
} ActionResponse;

void action_response_set(ActionResponse *res, int status, const char *body);

#endif /* ACTION_RESPONSE_H */

