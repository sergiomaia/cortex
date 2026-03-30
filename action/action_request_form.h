#ifndef ACTION_REQUEST_FORM_H
#define ACTION_REQUEST_FORM_H

#include <stddef.h>

/* Parse application/x-www-form-urlencoded bodies (e.g. HTML form POST). */

/* Decode one value for `key` into `out` (NUL-terminated). Returns value length (>=0)
 * if the key appears at least once, or -1 if absent. Empty values yield 0. */
int action_request_form_get(const char *body, const char *key, char *out, size_t out_cap);

/* 1 if the key appears in the body (e.g. checkbox checked), 0 otherwise. */
int action_request_form_present(const char *body, const char *key);

#endif /* ACTION_REQUEST_FORM_H */
