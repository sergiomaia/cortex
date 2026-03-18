#ifndef NEURAL_RETRIEVAL_H
#define NEURAL_RETRIEVAL_H

#include "neural_embedding.h"
#include "neural_memory.h"

typedef struct {
    char *keyword;
    NeuralEmbedding embedding;
} NeuralRetrievalEntry;

typedef struct {
    NeuralRetrievalEntry *entries;
    int count;
    int capacity;
} NeuralRetrieval;

void neural_retrieval_init(NeuralRetrieval *retrieval);
void neural_retrieval_free(NeuralRetrieval *retrieval);

/* Store or overwrite the embedding for a keyword.
 * Returns 0 on success, -1 on failure. */
int neural_retrieval_store(NeuralRetrieval *retrieval,
                           const char *keyword,
                           NeuralEmbedding embedding);

/* Perform cosine-similarity search over stored embeddings.
 * Writes up to top_k keyword pointers into out_keywords (may be NULL if not needed)
 * and similarity scores into out_scores (may be NULL). Returns the number of
 * results written, or -1 on error. */
int neural_retrieval_search(const NeuralRetrieval *retrieval,
                            NeuralEmbedding query,
                            int top_k,
                            const char **out_keywords,
                            float *out_scores);

/* Convenience: return the value from NeuralMemory whose keyword is most similar
 * to the query embedding. Returns NULL if no entries or on error. */
const char *neural_retrieval_best_value(const NeuralRetrieval *retrieval,
                                        const NeuralMemory *memory,
                                        NeuralEmbedding query);

#endif /* NEURAL_RETRIEVAL_H */

