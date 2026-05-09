#ifndef ACTION_ERROR_H
#define ACTION_ERROR_H

#include "action_response.h"
#include "cortex_error.h"

/*
 * If a thread-local Cortex error is present, renders an HTTP JSON error body,
 * clears the error, and returns non-zero so the caller can return early from
 * the action.
 */
int action_handle_error(ActionResponse *res);

void action_render_error(ActionResponse *res, const CortexError *err);

#define ACTION_HANDLE_ERROR(res) (action_handle_error(res))

#endif /* ACTION_ERROR_H */
