#include "guard_auth.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

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
        return NULL;
    }

    token = (char *)malloc((size_t)written + 1);
    if (!token) {
        return NULL;
    }

    memcpy(token, buffer, (size_t)written + 1);
    return token;
}

char *guard_auth_basic(GuardAuth *auth, const char *username, const char *password) {
    char *token;

    if (!auth || !auth->store || !username || !password) {
        return NULL;
    }

    if (!auth->expected_username || !auth->expected_password) {
        return NULL;
    }

    if (strcmp(username, auth->expected_username) != 0 ||
        strcmp(password, auth->expected_password) != 0) {
        return NULL;
    }

    token = guard_auth_generate_token();
    if (!token) {
        return NULL;
    }

    if (guard_session_create(auth->store, username, token) != 0) {
        free(token);
        return NULL;
    }

    return token;
}

