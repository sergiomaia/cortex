#include <string.h>

#include "../test_assert.h"
#include "../../core/neural_embedding.h"
#include "../../core/neural_memory.h"
#include "../../core/neural_retrieval.h"

void test_neural_retrieval_store_and_search_top_k(void) {
    NeuralRetrieval retrieval;
    NeuralEmbedding emb_error;
    NeuralEmbedding emb_warning;
    NeuralEmbedding emb_info;
    NeuralEmbedding query;
    const char *keywords[2];
    float scores[2];
    int n;
    int i;

    neural_retrieval_init(&retrieval);

    /* Build deterministic embeddings:
     * - error:   [1, 0, 0, ...]
     * - warning: [0, 1, 0, ...]
     * - info:    [1, 1, 0, ...]
     * Query equals error. */
    for (i = 0; i < NEURAL_EMBEDDING_DIM; ++i) {
        emb_error.values[i] = 0.0f;
        emb_warning.values[i] = 0.0f;
        emb_info.values[i] = 0.0f;
    }
    emb_error.values[0] = 1.0f;
    emb_warning.values[1] = 1.0f;
    emb_info.values[0] = 1.0f;
    emb_info.values[1] = 1.0f;

    ASSERT_EQ(neural_retrieval_store(&retrieval, "error", emb_error), 0);
    ASSERT_EQ(neural_retrieval_store(&retrieval, "warning", emb_warning), 0);
    ASSERT_EQ(neural_retrieval_store(&retrieval, "info", emb_info), 0);

    query = emb_error;

    n = neural_retrieval_search(&retrieval, query, 2, keywords, scores);
    ASSERT_EQ(n, 2);

    for (i = 0; i < n; ++i) {
        ASSERT_TRUE(keywords[i] != NULL);
    }

    /* Top-2 by cosine similarity should be:
     * - "error" (cosine 1.0)
     * - "info"  (cosine 1/sqrt(2) ~ 0.707)
     * "warning" has cosine 0.0 with the query. */
    ASSERT_TRUE(strcmp(keywords[0], "error") == 0 || strcmp(keywords[1], "error") == 0);
    ASSERT_TRUE(strcmp(keywords[0], "info") == 0 || strcmp(keywords[1], "info") == 0);

    neural_retrieval_free(&retrieval);
}

void test_neural_retrieval_integrates_with_memory(void) {
    NeuralRetrieval retrieval;
    NeuralMemory memory;
    NeuralEmbedding emb_error;
    NeuralEmbedding emb_warning;
    NeuralEmbedding query;
    const char *value;

    neural_retrieval_init(&retrieval);
    neural_memory_init(&memory);

    for (int i = 0; i < NEURAL_EMBEDDING_DIM; ++i) {
        emb_error.values[i] = 0.0f;
        emb_warning.values[i] = 0.0f;
    }
    emb_error.values[0] = 1.0f;
    emb_warning.values[1] = 1.0f;
    query = emb_error;

    ASSERT_EQ(neural_retrieval_store(&retrieval, "error", emb_error), 0);
    ASSERT_EQ(neural_retrieval_store(&retrieval, "warning", emb_warning), 0);

    ASSERT_EQ(neural_memory_store(&memory, "error", "Database connection failed"), 0);
    ASSERT_EQ(neural_memory_store(&memory, "warning", "Disk space low"), 0);

    value = neural_retrieval_best_value(&retrieval, &memory, query);
    ASSERT_TRUE(value != NULL);
    ASSERT_STR_EQ(value, "Database connection failed");

    neural_memory_free(&memory);
    neural_retrieval_free(&retrieval);
}

