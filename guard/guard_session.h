#ifndef GUARD_SESSION_H
#define GUARD_SESSION_H

#include <stddef.h>

typedef struct {
    char *token;
    char *user_id;
    int in_use;
} GuardSessionEntry;

typedef struct {
    GuardSessionEntry *entries;
    size_t capacity;
} GuardSessionStore;

/* Initialize an in-memory session store with the given capacity. Returns 0 on success. */
int guard_session_store_init(GuardSessionStore *store, size_t capacity);

/* Free all memory associated with the store. */
void guard_session_store_free(GuardSessionStore *store);

/* Create a new session for the given user and token. Returns 0 on success. */
int guard_session_create(GuardSessionStore *store, const char *user_id, const char *token);

/* Returns 1 if a session exists for the given token, 0 otherwise. */
int guard_session_validate(GuardSessionStore *store, const char *token);

/* Find the session entry for a given token, or NULL if not found. */
GuardSessionEntry *guard_session_find(GuardSessionStore *store, const char *token);

#endif /* GUARD_SESSION_H */

