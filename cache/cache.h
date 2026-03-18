#ifndef CACHE_H
#define CACHE_H

#include <stddef.h>
#include <time.h>

typedef struct {
    char *key;
    void *value;
    time_t expires_at; /* 0 means no expiration */
    int in_use;
} CacheEntry;

typedef struct {
    CacheEntry *entries;
    size_t capacity;
} Cache;

/* Initialize an in-memory cache with the given capacity. Returns 0 on success. */
int cache_memory_init(Cache *cache, size_t capacity);

/* Free any resources held by the cache. Does not free stored values. */
void cache_memory_free(Cache *cache);

/*
 * Set a key/value pair in the cache.
 *
 * ttl_seconds > 0 => entry expires now + ttl_seconds
 * ttl_seconds <= 0 => no expiration
 *
 * Returns 0 on success, -1 on failure (e.g. cache full).
 */
int cache_set(Cache *cache, const char *key, void *value, int ttl_seconds);

/*
 * Get a value by key.
 *
 * Returns the stored value pointer or NULL if the key is missing or expired.
 */
void *cache_get(Cache *cache, const char *key);

#endif /* CACHE_H */

