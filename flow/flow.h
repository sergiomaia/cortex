#ifndef FLOW_H
#define FLOW_H

/* Flow system:
 * - FlowJob: unit of work (function + user_data)
 * - FlowQueue: in-memory FIFO queue (no threads)
 * - FlowWorker: sequential dispatcher that drains the queue
 */

typedef void (*FlowJobFn)(void *user_data);

typedef struct {
    FlowJobFn fn;
    void *user_data;
} FlowJob;

typedef struct {
    FlowJob *items;
    int capacity;
    int head; /* index of next element to dequeue */
    int tail; /* index of next element to enqueue */
    int count;
} FlowQueue;

typedef struct {
    FlowQueue *queue;
} FlowWorker;

void flow_queue_init(FlowQueue *queue);
void flow_queue_free(FlowQueue *queue);
int flow_queue_enqueue(FlowQueue *queue, FlowJob job);
int flow_queue_dequeue(FlowQueue *queue, FlowJob *out);

void flow_worker_init(FlowWorker *worker, FlowQueue *queue);
int flow_worker_dispatch(FlowWorker *worker);

#endif /* FLOW_H */

