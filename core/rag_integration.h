#ifndef RAG_INTEGRATION_H
#define RAG_INTEGRATION_H

#include "neural_runtime.h"
#include "neural_memory.h"
#include "neural_retrieval.h"
#include "neural_embedding.h"

/* Minimal RAG engine that composes:
 * - NeuralRuntime for generation
 * - NeuralMemory for keyword->value storage
 * - NeuralRetrieval for embedding-based lookup
 *
 * This keeps the existing NeuralRuntime API untouched while still
 * providing a simple retrieval-augmented generation flow.
 */
typedef struct {
    NeuralRuntime runtime;
    NeuralMemory memory;
    NeuralRetrieval retrieval;
} RAGEngine;

/* Initialize the engine with a given model name. */
void rag_engine_init(RAGEngine *e, const char *model_name);

/* Store a memory value under a keyword, and index its embedding
 * for later retrieval. Returns 0 on success, -1 on error.
 */
int rag_engine_store(RAGEngine *e, const char *keyword, const char *value);

/* Run a retrieval-augmented query.
 * - Embeds the query
 * - Retrieves the most relevant memory value (if any)
 * - Builds a simple prompt that includes the context and question
 * - Calls neural_run and writes the final response into `out`
 *
 * Returns 0 on success, -1 on error.
 */
int rag_engine_query(RAGEngine *e,
                     const char *question,
                     char *out,
                     int out_size);

#endif /* RAG_INTEGRATION_H */

