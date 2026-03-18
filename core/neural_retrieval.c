#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "neural_retrieval.h"

static char *dup_string(const char *s) {
    size_t n;
    char *out;
    if (!s) {
        s = "";
    }
    n = strlen(s) + 1;
    out = (char *)malloc(n);
    if (!out) {
        return NULL;
    }
    memcpy(out, s, n);
    return out;
}

static int ensure_capacity(NeuralRetrieval *retrieval) {
    int new_capacity;
    NeuralRetrievalEntry *new_entries;
    int i;

    if (!retrieval) {
        return -1;
    }

    if (retrieval->capacity == 0) {
        new_capacity = 4;
    } else if (retrieval->count < retrieval->capacity) {
        return 0;
    } else {
        new_capacity = retrieval->capacity * 2;
    }

    new_entries = (NeuralRetrievalEntry *)realloc(
        retrieval->entries, (size_t)new_capacity * sizeof(NeuralRetrievalEntry));
    if (!new_entries) {
        return -1;
    }

    for (i = retrieval->capacity; i < new_capacity; ++i) {
        new_entries[i].keyword = NULL;
    }

    retrieval->entries = new_entries;
    retrieval->capacity = new_capacity;
    return 0;
}

void neural_retrieval_init(NeuralRetrieval *retrieval) {
    if (!retrieval) {
        return;
    }
    retrieval->entries = NULL;
    retrieval->count = 0;
    retrieval->capacity = 0;
}

void neural_retrieval_free(NeuralRetrieval *retrieval) {
    int i;
    if (!retrieval) {
        return;
    }
    for (i = 0; i < retrieval->count; ++i) {
        free(retrieval->entries[i].keyword);
        retrieval->entries[i].keyword = NULL;
    }
    free(retrieval->entries);
    retrieval->entries = NULL;
    retrieval->count = 0;
    retrieval->capacity = 0;
}

int neural_retrieval_store(NeuralRetrieval *retrieval,
                           const char *keyword,
                           NeuralEmbedding embedding) {
    int i;
    char *kcopy;

    if (!retrieval || !keyword) {
        return -1;
    }

    for (i = 0; i < retrieval->count; ++i) {
        if (retrieval->entries[i].keyword &&
            strcmp(retrieval->entries[i].keyword, keyword) == 0) {
            retrieval->entries[i].embedding = embedding;
            return 0;
        }
    }

    if (ensure_capacity(retrieval) != 0) {
        return -1;
    }

    kcopy = dup_string(keyword);
    if (!kcopy) {
        return -1;
    }

    retrieval->entries[retrieval->count].keyword = kcopy;
    retrieval->entries[retrieval->count].embedding = embedding;
    retrieval->count += 1;

    return 0;
}

static float cosine_similarity(const NeuralEmbedding *a,
                               const NeuralEmbedding *b) {
    int i;
    float dot = 0.0f;
    float na = 0.0f;
    float nb = 0.0f;

    if (!a || !b) {
        return 0.0f;
    }

    for (i = 0; i < NEURAL_EMBEDDING_DIM; ++i) {
        float va = a->values[i];
        float vb = b->values[i];
        dot += va * vb;
        na += va * va;
        nb += vb * vb;
    }

    if (na == 0.0f || nb == 0.0f) {
        return 0.0f;
    }

    return dot / (sqrtf(na) * sqrtf(nb));
}

int neural_retrieval_search(const NeuralRetrieval *retrieval,
                            NeuralEmbedding query,
                            int top_k,
                            const char **out_keywords,
                            float *out_scores) {
    int i, j;
    float *scores_buf = NULL;

    if (!retrieval || top_k <= 0) {
        return -1;
    }

    if (retrieval->count == 0) {
        return 0;
    }

    if (top_k > retrieval->count) {
        top_k = retrieval->count;
    }

    scores_buf = (float *)malloc((size_t)top_k * sizeof(float));
    if (!scores_buf) {
        return -1;
    }

    /* Initialize with the first top_k items. */
    for (i = 0; i < top_k; ++i) {
        float score = cosine_similarity(&query, &retrieval->entries[i].embedding);
        scores_buf[i] = score;
        if (out_keywords) {
            out_keywords[i] = retrieval->entries[i].keyword;
        }
        if (out_scores) {
            out_scores[i] = score;
        }
    }

    /* Maintain the current worst element in scores_buf. */
    for (i = top_k; i < retrieval->count; ++i) {
        float score = cosine_similarity(&query, &retrieval->entries[i].embedding);

        int min_index = 0;
        float min_score = scores_buf[0];
        for (j = 1; j < top_k; ++j) {
            if (scores_buf[j] < min_score) {
                min_score = scores_buf[j];
                min_index = j;
            }
        }

        if (score > min_score) {
            scores_buf[min_index] = score;
            if (out_keywords) {
                out_keywords[min_index] = retrieval->entries[i].keyword;
            }
            if (out_scores) {
                out_scores[min_index] = score;
            }
        }
    }

    free(scores_buf);
    return top_k;
}

const char *neural_retrieval_best_value(const NeuralRetrieval *retrieval,
                                        const NeuralMemory *memory,
                                        NeuralEmbedding query) {
    const char *keyword;
    const char *value;
    const char *keywords[1];
    int n;

    if (!retrieval || !memory) {
        return NULL;
    }

    n = neural_retrieval_search(retrieval, query, 1, keywords, NULL);
    if (n <= 0) {
        return NULL;
    }

    keyword = keywords[0];
    if (!keyword) {
        return NULL;
    }

    value = neural_memory_retrieve(memory, keyword);
    return value;
}

