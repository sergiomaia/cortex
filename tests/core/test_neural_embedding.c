#include "../test_assert.h"
#include "../../core/neural_embedding.h"

void test_neural_embedding_same_input_same_vector(void) {
    const char *text = "hello world";
    NeuralEmbedding e1 = neural_embedding_from_text(text);
    NeuralEmbedding e2 = neural_embedding_from_text(text);
    size_t i;

    for (i = 0; i < NEURAL_EMBEDDING_DIM; ++i) {
        ASSERT_TRUE(e1.values[i] == e2.values[i]);
    }
}

void test_neural_embedding_different_input_different_vector(void) {
    const char *text1 = "hello world";
    const char *text2 = "goodbye world";
    NeuralEmbedding e1 = neural_embedding_from_text(text1);
    NeuralEmbedding e2 = neural_embedding_from_text(text2);
    size_t i;
    int any_diff = 0;

    for (i = 0; i < NEURAL_EMBEDDING_DIM; ++i) {
        if (e1.values[i] != e2.values[i]) {
            any_diff = 1;
            break;
        }
    }

    ASSERT_TRUE(any_diff);
}

void test_neural_embedding_generates_vector_from_text(void) {
    const char *text = "embedding test";
    NeuralEmbedding e = neural_embedding_from_text(text);
    size_t i;
    int any_non_zero = 0;

    for (i = 0; i < NEURAL_EMBEDDING_DIM; ++i) {
        if (e.values[i] != 0.0f) {
            any_non_zero = 1;
            break;
        }
    }

    ASSERT_TRUE(any_non_zero);
}

