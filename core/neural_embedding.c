#include <stdint.h>
#include <limits.h>

#include "neural_embedding.h"

/* Simple deterministic hash-based mock embedding.
 * For each dimension we use a slightly different seed and run a
 * FNV-1a style hash over the input text, then map the resulting
 * 32-bit integer into the [-1.0, 1.0] float range. */

static float hash_to_unit_interval(uint32_t h) {
    /* Map unsigned 32-bit integer to [0,1]. UINT32_MAX may not be defined
     * on some systems, so we use the literal 4294967295u. */
    const float scale = 1.0f / 4294967295.0f;
    return (float)h * scale;
}

NeuralEmbedding neural_embedding_from_text(const char *text) {
    NeuralEmbedding emb;
    size_t i;

    if (text == NULL) {
        text = "";
    }

    for (i = 0; i < NEURAL_EMBEDDING_DIM; ++i) {
        uint32_t h = 2166136261u ^ (uint32_t)(i * 16777619u);
        const unsigned char *p = (const unsigned char *)text;

        while (*p) {
            h ^= (uint32_t)(*p++);
            h *= 16777619u;
        }

        /* Map to [-1, 1]. */
        float u = hash_to_unit_interval(h);
        emb.values[i] = (u * 2.0f) - 1.0f;
    }

    return emb;
}

