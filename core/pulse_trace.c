#include "pulse_trace.h"

void pulse_trace_start(PulseTrace *trace) {
    if (!trace) {
        return;
    }
    trace->active = 1;
    trace->start_clock = clock();
    trace->elapsed_ms = 0.0;
}

void pulse_trace_stop(PulseTrace *trace) {
    clock_t now;
    clock_t diff;

    if (!trace || !trace->active) {
        return;
    }

    now = clock();
    diff = now - trace->start_clock;

    if (diff < 0) {
        trace->elapsed_ms = 0.0;
    } else {
        trace->elapsed_ms = (double)diff * 1000.0 / (double)CLOCKS_PER_SEC;
    }

    trace->active = 0;
}

double pulse_trace_elapsed_ms(const PulseTrace *trace) {
    if (!trace) {
        return 0.0;
    }
    return trace->elapsed_ms;
}

