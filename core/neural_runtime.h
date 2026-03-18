#ifndef NEURAL_RUNTIME_H
#define NEURAL_RUNTIME_H

#include "neural_model.h"

typedef struct {
    NeuralModel model;
} NeuralRuntime;

NeuralRuntime neural_runtime_init(const char *model_name);
const char *neural_run(NeuralRuntime *rt, const char *prompt);

#endif /* NEURAL_RUNTIME_H */

