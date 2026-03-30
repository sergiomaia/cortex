#ifndef ACTION_RESPONSE_H
#define ACTION_RESPONSE_H

typedef struct {
    int status;
    const char *body;
    const char *content_type;
    /* If non-NULL, HTTP response includes Location (e.g. redirect after POST). */
    const char *location;
} ActionResponse;

void action_response_set(ActionResponse *res, int status, const char *body);

/* Optional: override the default content type (text/plain). */
void action_response_set_content_type(ActionResponse *res, const char *content_type);

#endif /* ACTION_RESPONSE_H */

