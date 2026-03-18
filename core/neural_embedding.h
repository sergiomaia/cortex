#ifndef NEURAL_EMBEDDING_H
#define NEURAL_EMBEDDING_H

#include <stddef.h>

/* Fixed dimensionality for mock embeddings. */
#define NEURAL_EMBEDDING_DIM 16

typedef struct {
    float values[NEURAL_EMBEDDING_DIM];
} NeuralEmbedding;

/* Generate a deterministic mock embedding vector for the given text. */
NeuralEmbedding neural_embedding_from_text(const char *text);

#endif /* NEURAL_EMBEDDING_H */

