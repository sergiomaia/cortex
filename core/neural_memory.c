#include <stdlib.h>
#include <string.h>

#include "neural_memory.h"

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

static int ensure_capacity(NeuralMemory *mem) {
    int new_capacity;
    NeuralMemoryEntry *new_entries;
    int i;

    if (!mem) {
        return -1;
    }

    if (mem->capacity == 0) {
        new_capacity = 4;
    } else if (mem->count < mem->capacity) {
        return 0;
    } else {
        new_capacity = mem->capacity * 2;
    }

    new_entries = (NeuralMemoryEntry *)realloc(mem->entries, new_capacity * sizeof(NeuralMemoryEntry));
    if (!new_entries) {
        return -1;
    }

    /* Zero newly allocated slots to keep cleanup safe. */
    for (i = mem->capacity; i < new_capacity; ++i) {
        new_entries[i].keyword = NULL;
        new_entries[i].value = NULL;
    }

    mem->entries = new_entries;
    mem->capacity = new_capacity;
    return 0;
}

void neural_memory_init(NeuralMemory *mem) {
    if (!mem) {
        return;
    }
    mem->entries = NULL;
    mem->count = 0;
    mem->capacity = 0;
}

int neural_memory_store(NeuralMemory *mem, const char *keyword, const char *value) {
    int i;
    const char *v = value ? value : "";
    char *kcopy;
    char *vcopy;

    if (!mem || !keyword) {
        return -1;
    }

    /* Overwrite if keyword already exists. */
    for (i = 0; i < mem->count; ++i) {
        if (mem->entries[i].keyword && strcmp(mem->entries[i].keyword, keyword) == 0) {
            char *copy = dup_string(v);
            if (!copy) {
                return -1;
            }
            free(mem->entries[i].value);
            mem->entries[i].value = copy;
            return 0;
        }
    }

    if (ensure_capacity(mem) != 0) {
        return -1;
    }

    kcopy = dup_string(keyword);
    vcopy = dup_string(v);
    if (!kcopy || !vcopy) {
        free(kcopy);
        free(vcopy);
        return -1;
    }
    mem->entries[mem->count].keyword = kcopy;
    mem->entries[mem->count].value = vcopy;

    mem->count += 1;
    return 0;
}

const char *neural_memory_retrieve(const NeuralMemory *mem, const char *keyword) {
    int i;

    if (!mem || !keyword) {
        return NULL;
    }

    for (i = 0; i < mem->count; ++i) {
        if (mem->entries[i].keyword && strcmp(mem->entries[i].keyword, keyword) == 0) {
            return mem->entries[i].value;
        }
    }

    return NULL;
}

void neural_memory_free(NeuralMemory *mem) {
    int i;

    if (!mem) {
        return;
    }

    for (i = 0; i < mem->count; ++i) {
        free(mem->entries[i].keyword);
        free(mem->entries[i].value);
        mem->entries[i].keyword = NULL;
        mem->entries[i].value = NULL;
    }

    free(mem->entries);
    mem->entries = NULL;
    mem->count = 0;
    mem->capacity = 0;
}

