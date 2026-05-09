#include "main.h"

#include "pulse.h"

#ifndef CORTEX_VERSION
#define CORTEX_VERSION "0.0.0"
#endif

void cortex_main_bootstrap(void)
{
    pulse_init_from_env();
    pulse_log_fields(PULSE_INFO,
                     "cortex",
                     "framework starting",
                     "version", CORTEX_VERSION,
                     NULL);
}

void cortex_main_shutdown(void)
{
    pulse_log(PULSE_INFO, "cortex", "framework shutting down");
    pulse_shutdown();
}
