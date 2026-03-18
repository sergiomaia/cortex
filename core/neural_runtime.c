#include <string.h>
#include <stdint.h>
#include <stdio.h>

#include "neural_runtime.h"

static uint64_t hash_prompt_djb2(const char *s) {
    /* Deterministic djb2-like hash for prompt strings. */
    uint64_t hash = 5381;
    int c;
    while (s && (c = (unsigned char)*s++) != 0) {
        hash = ((hash << 5) + hash) + (uint64_t)(unsigned char)c;
    }
    return hash;
}

static void prompt_cache_key(const char *prompt, char *out, size_t out_size) {
    uint64_t h;
    if (!out || out_size == 0) {
        return;
    }
    if (!prompt) {
        prompt = "";
    }
    h = hash_prompt_djb2(prompt);
    (void)snprintf(out, out_size, "prompt:%016llx",
                    (unsigned long long)h);
}

NeuralRuntime neural_runtime_init(const char *model_name) {
    NeuralRuntime rt;
    rt.model.name = model_name;
    rt.cache_enabled = 0;
    rt.cache.entries = NULL;
    rt.cache.capacity = 0;
    rt.compute_calls = 0;
    return rt;
}

NeuralRuntime neural_runtime_init_cached(const char *model_name,
                                         int enable_cache,
                                         size_t cache_capacity) {
    NeuralRuntime rt;
    int ok = 0;

    rt.model.name = model_name;
    rt.cache_enabled = 0;
    rt.cache.entries = NULL;
    rt.cache.capacity = 0;
    rt.compute_calls = 0;

    if (enable_cache && cache_capacity > 0) {
        ok = (cache_memory_init(&rt.cache, cache_capacity) == 0);
    }

    rt.cache_enabled = ok ? 1 : 0;
    return rt;
}

const char *neural_run(NeuralRuntime *rt, const char *prompt) {
    char key[64];
    const char *cached;
    const char *resp;

    if (!rt || !prompt) {
        return NULL;
    }

    if (rt->cache_enabled) {
        prompt_cache_key(prompt, key, sizeof(key));
        cached = (const char *)cache_get(&rt->cache, key);
        if (cached) {
            return cached;
        }
    }

    /* Mock "inference": increment only when we actually recompute. */
    rt->compute_calls += 1;
    resp = "mock-response";

    if (rt->cache_enabled) {
        (void)prompt_cache_key(prompt, key, sizeof(key));
        /* Store pointer value without taking ownership. */
        (void)cache_set(&rt->cache, key, (void *)resp, 0);
    }

    return resp;
}

