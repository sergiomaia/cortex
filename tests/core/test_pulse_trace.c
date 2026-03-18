#include <time.h>
#include "../test_assert.h"
#include "../../core/pulse_trace.h"

void test_pulse_trace_duration_positive(void) {
    PulseTrace trace;
    double elapsed;
    clock_t start;

    pulse_trace_start(&trace);
    start = trace.start_clock;

    /* Busy-wait until clock() advances by at least 1 tick. */
    while ((clock() - start) < 1) {
        /* Prevent the loop from being optimized away. */
        __asm__ volatile("" ::: "memory");
    }

    pulse_trace_stop(&trace);
    elapsed = pulse_trace_elapsed_ms(&trace);

    ASSERT_TRUE(elapsed > 0.0);
}

