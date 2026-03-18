#ifndef ACTION_REQUEST_H
#define ACTION_REQUEST_H

typedef struct {
    const char *method;
    const char *path;
    const char *body;
} ActionRequest;

#endif /* ACTION_REQUEST_H */

