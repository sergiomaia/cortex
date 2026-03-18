#ifndef CORE_APP_H
#define CORE_APP_H

typedef struct {
    int initialized;
} CoreApp;

CoreApp core_app_init(void);
void core_app_shutdown(CoreApp *app);

#endif /* CORE_APP_H */

