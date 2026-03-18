#include <string.h>

#include "neural_runtime.h"

NeuralRuntime neural_runtime_init(const char *model_name) {
    NeuralRuntime rt;
    rt.model.name = model_name;
    return rt;
}

const char *neural_run(NeuralRuntime *rt, const char *prompt) {
    (void)rt;
    (void)prompt;
    /* Mock implementation: in a real system this would call into a
     * neural inference engine. For now we simply return a constant
     * response to enable testing of wiring and call semantics. */
    return "mock-response";
}

