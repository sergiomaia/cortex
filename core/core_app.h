#ifndef CORE_APP_H
#define CORE_APP_H

typedef struct {
    int initialized;
} CoreApp;

/* Initialize the application.
 * Returns a CoreApp instance with initialized flag set.
 * In a real application, this would wire config, env and logging.
 */
CoreApp core_app_init(void);

void core_app_shutdown(CoreApp *app);

#endif /* CORE_APP_H */

