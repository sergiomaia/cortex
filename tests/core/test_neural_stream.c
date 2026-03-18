#include <string.h>

#include "../test_assert.h"
#include "../../core/neural_stream.h"

static char streamed_tokens[4][64];
static int streamed_count = 0;
static int last_is_last_flag = 0;

static void capture_callback(const char *token, int is_last, void *user_data) {
    (void)user_data;

    if (streamed_count < 4) {
        strncpy(streamed_tokens[streamed_count], token, sizeof(streamed_tokens[0]) - 1);
        streamed_tokens[streamed_count][sizeof(streamed_tokens[0]) - 1] = '\0';
        streamed_count += 1;
    }

    last_is_last_flag = is_last;
}

void test_neural_stream_calls_callback_multiple_times(void) {
    NeuralRuntime rt = neural_runtime_init("gpt-stream-mock");
    NeuralStream stream = neural_stream_init(rt);

    streamed_count = 0;
    last_is_last_flag = 0;

    ASSERT_TRUE(neural_stream_run(&stream, "any prompt", capture_callback, NULL) >= 2);
    ASSERT_EQ(streamed_count, 2);
    ASSERT_EQ(last_is_last_flag, 1);

    /* With mock response "mock-response", our tokenizer splits on '-' */
    ASSERT_STR_EQ(streamed_tokens[0], "mock");
    ASSERT_STR_EQ(streamed_tokens[1], "response");
}

