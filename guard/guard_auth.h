#ifndef GUARD_AUTH_H
#define GUARD_AUTH_H

#include "guard_session.h"

typedef struct {
    const char *expected_username;
    const char *expected_password;
    GuardSessionStore *store;
} GuardAuth;

/* Initialize a GuardAuth instance with a backing in-memory session store. */
void guard_auth_init(GuardAuth *auth,
                     GuardSessionStore *store,
                     const char *expected_username,
                     const char *expected_password);

/*
 * Perform a basic auth check. When the provided username/password match the
 * expected values, a new session is created and a non-NULL token string is
 * returned. The token is owned by the session store and remains valid until
 * guard_session_store_free is called.
 *
 * Returns NULL when authentication fails.
 */
char *guard_auth_basic(GuardAuth *auth, const char *username, const char *password);

#endif /* GUARD_AUTH_H */

