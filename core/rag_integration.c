#include <stdio.h>
#include <string.h>

#include "rag_integration.h"

void rag_engine_init(RAGEngine *e, const char *model_name) {
    if (!e) {
        return;
    }
    e->runtime = neural_runtime_init(model_name);
    neural_memory_init(&e->memory);
    neural_retrieval_init(&e->retrieval);
}

int rag_engine_store(RAGEngine *e, const char *keyword, const char *value) {
    NeuralEmbedding emb;

    if (!e || !keyword) {
        return -1;
    }

    /* Store raw value in memory. */
    if (neural_memory_store(&e->memory, keyword, value) != 0) {
        return -1;
    }

    /* Index deterministic embedding for retrieval. */
    emb = neural_embedding_from_text(keyword);
    if (neural_retrieval_store(&e->retrieval, keyword, emb) != 0) {
        return -1;
    }

    return 0;
}

int rag_engine_query(RAGEngine *e,
                     const char *question,
                     char *out,
                     int out_size) {
    NeuralEmbedding q_emb;
    const char *ctx_value = NULL;
    const char *resp;
    char prompt[512];

    if (!e || !question || !out || out_size <= 0) {
        return -1;
    }

    q_emb = neural_embedding_from_text(question);
    ctx_value = neural_retrieval_best_value(&e->retrieval,
                                            &e->memory,
                                            q_emb);

    if (ctx_value && ctx_value[0] != '\0') {
        /* Simple RAG-style prompt: include retrieved context before question. */
        if (snprintf(prompt,
                     (int)sizeof(prompt),
                     "Context: %s\n\nQuestion: %s",
                     ctx_value,
                     question) < 0) {
            return -1;
        }
    } else {
        /* Fall back to just the question. */
        if (snprintf(prompt,
                     (int)sizeof(prompt),
                     "Question: %s",
                     question) < 0) {
            return -1;
        }
    }

    resp = neural_run(&e->runtime, prompt);
    if (!resp) {
        return -1;
    }

    if (snprintf(out, out_size, "%s", resp) < 0) {
        return -1;
    }

    return 0;
}

