#ifndef CORTEX_MAIN_H
#define CORTEX_MAIN_H

/*
 * Application bootstrap and shutdown hooks.
 *
 * cortex_main_bootstrap() must be the first call performed by any executable
 * that links libcortex. It initializes the structured logging subsystem
 * (Pulse) using the runtime environment and emits a startup event.
 *
 * cortex_main_shutdown() flushes pending logs, emits a final shutdown event,
 * and releases any owned file handles. It is safe to call exactly once
 * before exit; double-shutdown is also safe but does nothing extra.
 */

#ifdef __cplusplus
extern "C" {
#endif

void cortex_main_bootstrap(void);
void cortex_main_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* CORTEX_MAIN_H */
