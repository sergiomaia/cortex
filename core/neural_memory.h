#ifndef NEURAL_MEMORY_H
#define NEURAL_MEMORY_H

typedef struct {
    char *keyword;
    char *value;
} NeuralMemoryEntry;

typedef struct {
    NeuralMemoryEntry *entries;
    int count;
    int capacity;
} NeuralMemory;

void neural_memory_init(NeuralMemory *mem);

/* Store a value under a keyword.
 * If the keyword already exists, its value is overwritten.
 * Returns 0 on success, -1 on failure. */
int neural_memory_store(NeuralMemory *mem, const char *keyword, const char *value);

/* Retrieve a stored value by keyword.
 * Returns NULL if keyword is missing. */
const char *neural_memory_retrieve(const NeuralMemory *mem, const char *keyword);

void neural_memory_free(NeuralMemory *mem);

#endif /* NEURAL_MEMORY_H */

