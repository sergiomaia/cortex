#include <stdlib.h>

#include "flow.h"

static int flow_queue_grow(FlowQueue *queue, int new_capacity) {
    FlowJob *new_items;
    int i;

    if (!queue || new_capacity <= 0) {
        return -1;
    }

    new_items = (FlowJob *)malloc((size_t)new_capacity * sizeof(FlowJob));
    if (!new_items) {
        return -1;
    }

    /* Copy existing items into a contiguous array in FIFO order. */
    for (i = 0; i < queue->count; ++i) {
        new_items[i] = queue->items[(queue->head + i) % queue->capacity];
    }

    free(queue->items);
    queue->items = new_items;
    queue->capacity = new_capacity;
    queue->head = 0;
    queue->tail = queue->count;

    return 0;
}

void flow_queue_init(FlowQueue *queue) {
    if (!queue) {
        return;
    }

    queue->items = NULL;
    queue->capacity = 0;
    queue->head = 0;
    queue->tail = 0;
    queue->count = 0;
}

void flow_queue_free(FlowQueue *queue) {
    if (!queue) {
        return;
    }

    free(queue->items);
    queue->items = NULL;
    queue->capacity = 0;
    queue->head = 0;
    queue->tail = 0;
    queue->count = 0;
}

int flow_queue_enqueue(FlowQueue *queue, FlowJob job) {
    int rc;
    int insert_idx;

    if (!queue || !job.fn) {
        return -1;
    }

    if (queue->count == queue->capacity) {
        int new_capacity = queue->capacity == 0 ? 4 : queue->capacity * 2;
        rc = flow_queue_grow(queue, new_capacity);
        if (rc != 0) {
            return -1;
        }
    }

    insert_idx = queue->tail;
    queue->items[insert_idx] = job;
    queue->tail = (queue->tail + 1) % queue->capacity;
    queue->count += 1;
    return 0;
}

int flow_queue_dequeue(FlowQueue *queue, FlowJob *out) {
    FlowJob item;

    if (!queue || !out) {
        return -1;
    }

    if (queue->count <= 0) {
        return -1;
    }

    item = queue->items[queue->head];
    queue->head = (queue->head + 1) % queue->capacity;
    queue->count -= 1;

    *out = item;
    return 0;
}

void flow_worker_init(FlowWorker *worker, FlowQueue *queue) {
    if (!worker) {
        return;
    }
    worker->queue = queue;
}

int flow_worker_dispatch(FlowWorker *worker) {
    int executed = 0;
    FlowJob job;

    if (!worker || !worker->queue) {
        return -1;
    }

    /* Sequential FIFO execution: dequeue and run each job in order. */
    while (flow_queue_dequeue(worker->queue, &job) == 0) {
        job.fn(job.user_data);
        executed += 1;
    }

    return executed;
}

