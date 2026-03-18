#include "../test_assert.h"
#include "../../flow/flow.h"

typedef struct {
    int *cursor;
    int *out;
    int id;
} OrderCtx;

static void record_job(void *user_data) {
    OrderCtx *ctx = (OrderCtx *)user_data;
    int idx = *ctx->cursor;
    ctx->out[idx] = ctx->id;
    *ctx->cursor = idx + 1;
}

void test_flow_queue_enqueue_and_worker_dispatch_executes_jobs(void) {
    FlowQueue queue;
    FlowWorker worker;
    int executed = 0;

    void inc_job(void *user_data) {
        (void)user_data;
        executed += 1;
    }

    flow_queue_init(&queue);
    flow_worker_init(&worker, &queue);

    ASSERT_EQ(flow_queue_enqueue(&queue, (FlowJob){inc_job, NULL}), 0);

    ASSERT_EQ(flow_worker_dispatch(&worker), 1);
    ASSERT_EQ(executed, 1);

    /* Second dispatch should do nothing (queue drained). */
    ASSERT_EQ(flow_worker_dispatch(&worker), 0);

    flow_queue_free(&queue);
}

void test_flow_queue_maintains_execution_order(void) {
    FlowQueue queue;
    FlowWorker worker;
    int out[3];
    int cursor;

    OrderCtx c1;
    OrderCtx c2;
    OrderCtx c3;

    out[0] = -1;
    out[1] = -1;
    out[2] = -1;
    cursor = 0;

    c1.cursor = &cursor;
    c1.out = out;
    c1.id = 1;

    c2.cursor = &cursor;
    c2.out = out;
    c2.id = 2;

    c3.cursor = &cursor;
    c3.out = out;
    c3.id = 3;

    flow_queue_init(&queue);
    flow_worker_init(&worker, &queue);

    /* Enqueue in FIFO order and ensure dispatch preserves it. */
    ASSERT_EQ(flow_queue_enqueue(&queue, (FlowJob){record_job, &c1}), 0);
    ASSERT_EQ(flow_queue_enqueue(&queue, (FlowJob){record_job, &c2}), 0);
    ASSERT_EQ(flow_queue_enqueue(&queue, (FlowJob){record_job, &c3}), 0);

    ASSERT_EQ(flow_worker_dispatch(&worker), 3);
    ASSERT_EQ(cursor, 3);
    ASSERT_EQ(out[0], 1);
    ASSERT_EQ(out[1], 2);
    ASSERT_EQ(out[2], 3);

    flow_queue_free(&queue);
}

