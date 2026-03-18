#ifndef NEURAL_STREAM_H
#define NEURAL_STREAM_H

#include "neural_runtime.h"

typedef void (*NeuralStreamCallback)(const char *token, int is_last, void *user_data);

typedef struct {
    NeuralRuntime runtime;
} NeuralStream;

NeuralStream neural_stream_init(NeuralRuntime runtime);

/* Streams a (mock) response token-by-token by calling `cb`.
 * Returns number of tokens emitted on success, -1 on error. */
int neural_stream_run(NeuralStream *stream,
                       const char *prompt,
                       NeuralStreamCallback cb,
                       void *user_data);

#endif /* NEURAL_STREAM_H */

