#include "../test_assert.h"
#include "../../guard/guard_session.h"
#include "../../guard/guard_auth.h"

void test_guard_session_create_and_validate(void) {
    GuardSessionStore store;

    ASSERT_EQ(guard_session_store_init(&store, 8), 0);

    ASSERT_EQ(guard_session_create(&store, "user-1", "token-123"), 0);

    ASSERT_EQ(guard_session_validate(&store, "token-123"), 1);
    ASSERT_EQ(guard_session_validate(&store, "missing-token"), 0);

    {
        GuardSessionEntry *entry = guard_session_find(&store, "token-123");
        ASSERT_TRUE(entry != NULL);
        ASSERT_STR_EQ(entry->user_id, "user-1");
    }

    guard_session_store_free(&store);
}

void test_guard_auth_basic_success_and_failure(void) {
    GuardSessionStore store;
    GuardAuth auth;
    char *token;

    ASSERT_EQ(guard_session_store_init(&store, 8), 0);

    guard_auth_init(&auth, &store, "admin", "secret");

    token = guard_auth_basic(&auth, "wrong", "creds");
    ASSERT_TRUE(token == NULL);

    token = guard_auth_basic(&auth, "admin", "secret");
    ASSERT_TRUE(token != NULL);

    ASSERT_EQ(guard_session_validate(&store, token), 1);

    guard_session_store_free(&store);
}

