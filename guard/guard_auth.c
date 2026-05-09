#include "guard_auth.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../core/cortex_error.h"

static unsigned long guard_auth_next_id = 1;

void guard_auth_init(GuardAuth *auth,
                     GuardSessionStore *store,
                     const char *expected_username,
                     const char *expected_password) {
    if (!auth) {
        return;
    }

    auth->store = store;
    auth->expected_username = expected_username;
    auth->expected_password = expected_password;
}

static char *guard_auth_generate_token(void) {
    char buffer[32];
    int written = snprintf(buffer, sizeof(buffer), "sess-%lu", guard_auth_next_id++);
    char *token;

    if (written <= 0 || (size_t)written >= sizeof(buffer)) {
        CORTEX_SET_ERROR(CORTEX_ERR_UNKNOWN, "guard:guard_auth_generate_token",
                         "internal token buffer formatting failed");
        return NULL;
    }

    token = (char *)malloc((size_t)written + 1);
    if (!token) {
        CORTEX_SET_ERROR(CORTEX_ERR_CORE_OOM, "guard:guard_auth_generate_token",
                         "unable to allocate session token storage");
        return NULL;
    }

    memcpy(token, buffer, (size_t)written + 1);
    return token;
}

char *guard_auth_basic(GuardAuth *auth, const char *username, const char *password) {
    char *token;

    if (!auth || !auth->store || !username || !password) {
        CORTEX_SET_ERROR(CORTEX_ERR_INVALID_ARGUMENT, "guard:guard_auth_basic",
                         "GuardAuth must be configured with store and credentials");
        return NULL;
    }

    if (!auth->expected_username || !auth->expected_password) {
        CORTEX_SET_ERROR(CORTEX_ERR_GUARD_AUTH_FAILED, "guard:guard_auth_basic",
                         "expected credentials are not configured");
        return NULL;
    }

    if (strcmp(username, auth->expected_username) != 0 ||
        strcmp(password, auth->expected_password) != 0) {
        CORTEX_SET_ERROR(CORTEX_ERR_GUARD_AUTH_FAILED, "guard:guard_auth_basic",
                         "username/password pair rejected");
        return NULL;
    }

    token = guard_auth_generate_token();
    if (!token) {
        return NULL;
    }

    if (guard_session_create(auth->store, username, token) != 0) {
        free(token);
        CORTEX_SET_ERROR(CORTEX_ERR_ACTION_INTERNAL, "guard:guard_auth_basic",
                         "failed to persist authenticated session");
        return NULL;
    }

    cortex_clear_error();
    return token;
}

