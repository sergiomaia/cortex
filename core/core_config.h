#ifndef CORE_CONFIG_H
#define CORE_CONFIG_H

typedef struct {
    const char *environment;
    int port;
} CoreConfig;

CoreConfig core_config_default(void);

#endif /* CORE_CONFIG_H */

