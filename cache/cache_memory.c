#include "cache.h"

#include <stdlib.h>
#include <string.h>

static size_t hash_string(const char *s) {
    /* Simple djb2 hash */
    unsigned long hash = 5381;
    int c;
    while ((c = (unsigned char)*s++) != 0) {
        hash = ((hash << 5) + hash) + (unsigned long)c;
    }
    return (size_t)hash;
}

int cache_memory_init(Cache *cache, size_t capacity) {
    if (!cache || capacity == 0) {
        return -1;
    }

    cache->entries = (CacheEntry *)calloc(capacity, sizeof(CacheEntry));
    if (!cache->entries) {
        cache->capacity = 0;
        return -1;
    }

    cache->capacity = capacity;
    return 0;
}

void cache_memory_free(Cache *cache) {
    size_t i;

    if (!cache || !cache->entries) {
        return;
    }

    for (i = 0; i < cache->capacity; ++i) {
        if (cache->entries[i].in_use) {
            free(cache->entries[i].key);
            cache->entries[i].key = NULL;
            cache->entries[i].value = NULL;
            cache->entries[i].in_use = 0;
            cache->entries[i].expires_at = 0;
        }
    }

    free(cache->entries);
    cache->entries = NULL;
    cache->capacity = 0;
}

static int cache_find_index(Cache *cache, const char *key, size_t *out_index) {
    size_t i;
    size_t idx;

    if (!cache || !cache->entries || cache->capacity == 0 || !key) {
        return -1;
    }

    idx = hash_string(key) % cache->capacity;

    for (i = 0; i < cache->capacity; ++i) {
        size_t probe = (idx + i) % cache->capacity;
        CacheEntry *entry = &cache->entries[probe];

        if (!entry->in_use) {
            /* Empty slot - stop probing, key not present. */
            break;
        }

        if (entry->key && strcmp(entry->key, key) == 0) {
            *out_index = probe;
            return 0;
        }
    }

    return -1;
}

int cache_set(Cache *cache, const char *key, void *value, int ttl_seconds) {
    size_t i;
    size_t idx;
    size_t first_free = (size_t)-1;
    time_t now;

    if (!cache || !cache->entries || cache->capacity == 0 || !key) {
        return -1;
    }

    /* First, see if the key already exists. */
    if (cache_find_index(cache, key, &idx) == 0) {
        CacheEntry *entry = &cache->entries[idx];
        entry->value = value;
        if (ttl_seconds > 0) {
            now = time(NULL);
            entry->expires_at = now + (time_t)ttl_seconds;
        } else {
            entry->expires_at = 0;
        }
        return 0;
    }

    idx = hash_string(key) % cache->capacity;

    for (i = 0; i < cache->capacity; ++i) {
        size_t probe = (idx + i) % cache->capacity;
        CacheEntry *entry = &cache->entries[probe];

        if (!entry->in_use) {
            first_free = probe;
            break;
        }
    }

    if (first_free == (size_t)-1) {
        /* Cache is full. */
        return -1;
    }

    {
        CacheEntry *entry = &cache->entries[first_free];
        size_t key_len = strlen(key);
        char *key_copy = (char *)malloc(key_len + 1);

        if (!key_copy) {
            return -1;
        }

        memcpy(key_copy, key, key_len + 1);

        entry->key = key_copy;
        entry->value = value;
        entry->in_use = 1;

        if (ttl_seconds > 0) {
            now = time(NULL);
            entry->expires_at = now + (time_t)ttl_seconds;
        } else {
            entry->expires_at = 0;
        }
    }

    return 0;
}

void *cache_get(Cache *cache, const char *key) {
    size_t idx;
    CacheEntry *entry;

    if (!cache || !cache->entries || cache->capacity == 0 || !key) {
        return NULL;
    }

    if (cache_find_index(cache, key, &idx) != 0) {
        return NULL;
    }

    entry = &cache->entries[idx];

    if (entry->expires_at != 0) {
        time_t now = time(NULL);
        if (now >= entry->expires_at) {
            /* Expired: treat as missing. */
            return NULL;
        }
    }

    return entry->value;
}

