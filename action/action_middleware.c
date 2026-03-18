#include "action_middleware.h"

/* Simple global middleware chain used by action_dispatch.
 * This is intentionally minimal and not thread-safe. */

#define ACTION_MIDDLEWARE_MAX 16

static ActionMiddleware g_middlewares[ACTION_MIDDLEWARE_MAX];
static int g_middleware_count = 0;

static ActionHandler g_current_handler = 0;
static ActionRequest *g_current_req = 0;
static ActionResponse *g_current_res = 0;
static int g_current_index = 0;

static void action_middleware_next_internal(void);

void action_middleware_clear(void) {
    int i;
    for (i = 0; i < ACTION_MIDDLEWARE_MAX; ++i) {
        g_middlewares[i] = 0;
    }
    g_middleware_count = 0;
}

int action_middleware_add(ActionMiddleware mw) {
    if (!mw) {
        return -1;
    }
    if (g_middleware_count >= ACTION_MIDDLEWARE_MAX) {
        return -1;
    }
    g_middlewares[g_middleware_count++] = mw;
    return 0;
}

static void action_middleware_next_internal(void) {
    if (g_current_index < g_middleware_count) {
        ActionMiddleware mw = g_middlewares[g_current_index++];
        if (mw) {
            mw(g_current_req, g_current_res, action_middleware_next_internal);
        } else {
            action_middleware_next_internal();
        }
    } else if (g_current_handler) {
        g_current_handler(g_current_req, g_current_res);
    }
}

void action_middleware_dispatch(ActionHandler handler, ActionRequest *req, ActionResponse *res) {
    g_current_handler = handler;
    g_current_req = req;
    g_current_res = res;
    g_current_index = 0;

    action_middleware_next_internal();
}

