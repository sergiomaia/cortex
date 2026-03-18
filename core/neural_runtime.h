#ifndef NEURAL_RUNTIME_H
#define NEURAL_RUNTIME_H

#include "neural_model.h"
#include "cache.h"

typedef struct {
    NeuralModel model;
    int cache_enabled; /* Optional prompt->response caching */
    Cache cache;       /* In-memory cache keyed by prompt hash */
    unsigned long compute_calls; /* For tests: increments only on recomputation */
} NeuralRuntime;

NeuralRuntime neural_runtime_init(const char *model_name);
NeuralRuntime neural_runtime_init_cached(const char *model_name,
                                         int enable_cache,
                                         size_t cache_capacity);
const char *neural_run(NeuralRuntime *rt, const char *prompt);

#endif /* NEURAL_RUNTIME_H */

