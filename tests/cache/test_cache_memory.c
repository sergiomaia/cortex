#include "../test_assert.h"
#include "../../cache/cache.h"

void test_cache_memory_set_and_get_key(void) {
    Cache cache;
    const char *value = "hello";

    ASSERT_EQ(cache_memory_init(&cache, 16), 0);

    ASSERT_EQ(cache_set(&cache, "greeting", (void *)value, 0), 0);

    const char *out = (const char *)cache_get(&cache, "greeting");
    ASSERT_TRUE(out != NULL);
    ASSERT_STR_EQ(out, "hello");

    cache_memory_free(&cache);
}

void test_cache_memory_overwrite_value(void) {
    Cache cache;
    const char *v1 = "first";
    const char *v2 = "second";

    ASSERT_EQ(cache_memory_init(&cache, 16), 0);

    ASSERT_EQ(cache_set(&cache, "key", (void *)v1, 0), 0);
    ASSERT_EQ(cache_set(&cache, "key", (void *)v2, 0), 0);

    const char *out = (const char *)cache_get(&cache, "key");
    ASSERT_TRUE(out != NULL);
    ASSERT_STR_EQ(out, "second");

    cache_memory_free(&cache);
}

void test_cache_memory_optional_expiration(void) {
    Cache cache;
    const char *short_lived = "temp";
    const char *no_expire = "forever";

    ASSERT_EQ(cache_memory_init(&cache, 16), 0);

    /* Entry with TTL should expire. */
    ASSERT_EQ(cache_set(&cache, "temp_key", (void *)short_lived, 1), 0);

    /* Entry without TTL should not expire. */
    ASSERT_EQ(cache_set(&cache, "perm_key", (void *)no_expire, 0), 0);

    /* Immediately both should be readable. */
    ASSERT_STR_EQ((const char *)cache_get(&cache, "temp_key"), "temp");
    ASSERT_STR_EQ((const char *)cache_get(&cache, "perm_key"), "forever");

    /* Sleep long enough for temp_key to expire. */
    sleep(2);

    ASSERT_TRUE(cache_get(&cache, "temp_key") == NULL);
    ASSERT_STR_EQ((const char *)cache_get(&cache, "perm_key"), "forever");

    cache_memory_free(&cache);
}

