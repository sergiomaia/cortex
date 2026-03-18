#include "guard_session.h"

#include <stdlib.h>
#include <string.h>

int guard_session_store_init(GuardSessionStore *store, size_t capacity) {
    size_t i;

    if (!store || capacity == 0) {
        return -1;
    }

    store->entries = (GuardSessionEntry *)calloc(capacity, sizeof(GuardSessionEntry));
    if (!store->entries) {
        store->capacity = 0;
        return -1;
    }

    store->capacity = capacity;
    for (i = 0; i < capacity; ++i) {
        store->entries[i].token = NULL;
        store->entries[i].user_id = NULL;
        store->entries[i].in_use = 0;
    }

    return 0;
}

void guard_session_store_free(GuardSessionStore *store) {
    size_t i;

    if (!store || !store->entries) {
        return;
    }

    for (i = 0; i < store->capacity; ++i) {
        GuardSessionEntry *entry = &store->entries[i];
        if (entry->in_use) {
            free(entry->token);
            free(entry->user_id);
            entry->token = NULL;
            entry->user_id = NULL;
            entry->in_use = 0;
        }
    }

    free(store->entries);
    store->entries = NULL;
    store->capacity = 0;
}

static GuardSessionEntry *guard_session_find_internal(GuardSessionStore *store, const char *token) {
    size_t i;

    if (!store || !store->entries || !token) {
        return NULL;
    }

    for (i = 0; i < store->capacity; ++i) {
        GuardSessionEntry *entry = &store->entries[i];
        if (entry->in_use && entry->token && strcmp(entry->token, token) == 0) {
            return entry;
        }
    }

    return NULL;
}

GuardSessionEntry *guard_session_find(GuardSessionStore *store, const char *token) {
    return guard_session_find_internal(store, token);
}

int guard_session_create(GuardSessionStore *store, const char *user_id, const char *token) {
    size_t i;
    size_t free_index = (size_t)-1;

    if (!store || !store->entries || !user_id || !token) {
        return -1;
    }

    if (guard_session_find_internal(store, token) != NULL) {
        /* Session with this token already exists; treat as success. */
        return 0;
    }

    for (i = 0; i < store->capacity; ++i) {
        if (!store->entries[i].in_use) {
            free_index = i;
            break;
        }
    }

    if (free_index == (size_t)-1) {
        return -1;
    }

    {
        GuardSessionEntry *entry = &store->entries[free_index];
        size_t token_len = strlen(token);
        size_t user_len = strlen(user_id);

        entry->token = (char *)malloc(token_len + 1);
        entry->user_id = (char *)malloc(user_len + 1);

        if (!entry->token || !entry->user_id) {
            free(entry->token);
            free(entry->user_id);
            entry->token = NULL;
            entry->user_id = NULL;
            return -1;
        }

        memcpy(entry->token, token, token_len + 1);
        memcpy(entry->user_id, user_id, user_len + 1);
        entry->in_use = 1;
    }

    return 0;
}

int guard_session_validate(GuardSessionStore *store, const char *token) {
    return guard_session_find_internal(store, token) != NULL ? 1 : 0;
}

