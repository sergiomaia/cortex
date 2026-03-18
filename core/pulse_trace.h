#ifndef PULSE_TRACE_H
#define PULSE_TRACE_H

#include <time.h>

typedef struct {
    int active;
    clock_t start_clock;
    double elapsed_ms;
} PulseTrace;

/* Start or restart a trace. */
void pulse_trace_start(PulseTrace *trace);

/* Stop the trace and update elapsed_ms. Safe to call multiple times. */
void pulse_trace_stop(PulseTrace *trace);

/* Convenience accessor for the last measured duration in milliseconds. */
double pulse_trace_elapsed_ms(const PulseTrace *trace);

#endif /* PULSE_TRACE_H */

