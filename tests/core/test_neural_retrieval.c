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

    emb_error = neural_embedding_from_text("error");
    emb_warning = neural_embedding_from_text("warning");
    emb_info = neural_embedding_from_text("info");

    ASSERT_EQ(neural_retrieval_store(&retrieval, "error", emb_error), 0);
    ASSERT_EQ(neural_retrieval_store(&retrieval, "warning", emb_warning), 0);
    ASSERT_EQ(neural_retrieval_store(&retrieval, "info", emb_info), 0);

    query = neural_embedding_from_text("error happened");

    n = neural_retrieval_search(&retrieval, query, 2, keywords, scores);
    ASSERT_EQ(n, 2);

    for (i = 0; i < n; ++i) {
        ASSERT_TRUE(keywords[i] != NULL);
    }

    ASSERT_TRUE((strcmp(keywords[0], "error") == 0) ||
                (strcmp(keywords[1], "error") == 0));

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

    emb_error = neural_embedding_from_text("database error");
    emb_warning = neural_embedding_from_text("disk warning");

    ASSERT_EQ(neural_retrieval_store(&retrieval, "error", emb_error), 0);
    ASSERT_EQ(neural_retrieval_store(&retrieval, "warning", emb_warning), 0);

    ASSERT_EQ(neural_memory_store(&memory, "error", "Database connection failed"), 0);
    ASSERT_EQ(neural_memory_store(&memory, "warning", "Disk space low"), 0);

    query = neural_embedding_from_text("database failure");

    value = neural_retrieval_best_value(&retrieval, &memory, query);
    ASSERT_TRUE(value != NULL);
    ASSERT_STR_EQ(value, "Database connection failed");

    neural_memory_free(&memory);
    neural_retrieval_free(&retrieval);
}

