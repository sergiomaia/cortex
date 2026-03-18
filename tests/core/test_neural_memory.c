#include <string.h>

#include "../test_assert.h"
#include "../../core/neural_memory.h"

void test_neural_memory_store_and_retrieve_by_keyword(void) {
    NeuralMemory mem;
    const char *v1;
    const char *v2;
    const char *missing;

    neural_memory_init(&mem);

    ASSERT_EQ(neural_memory_store(&mem, "error", "Server crashed"), 0);
    ASSERT_EQ(neural_memory_store(&mem, "incident:1", "Outage detected"), 0);

    v1 = neural_memory_retrieve(&mem, "error");
    ASSERT_TRUE(v1 != NULL);
    ASSERT_STR_EQ(v1, "Server crashed");

    v2 = neural_memory_retrieve(&mem, "incident:1");
    ASSERT_TRUE(v2 != NULL);
    ASSERT_STR_EQ(v2, "Outage detected");

    missing = neural_memory_retrieve(&mem, "missing");
    ASSERT_TRUE(missing == NULL);

    neural_memory_free(&mem);
}

void test_neural_memory_retrieval_accuracy_overwrite(void) {
    NeuralMemory mem;
    const char *v;

    neural_memory_init(&mem);

    ASSERT_EQ(neural_memory_store(&mem, "keyword", "first"), 0);
    ASSERT_EQ(neural_memory_store(&mem, "keyword", "second"), 0);

    v = neural_memory_retrieve(&mem, "keyword");
    ASSERT_TRUE(v != NULL);
    ASSERT_STR_EQ(v, "second");

    neural_memory_free(&mem);
}

