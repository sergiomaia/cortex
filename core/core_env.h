#ifndef CORE_ENV_H
#define CORE_ENV_H

typedef struct {
    const char *name;
} CoreEnv;

CoreEnv core_env_current(void);

#endif /* CORE_ENV_H */

