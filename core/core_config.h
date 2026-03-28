#ifndef CORE_CONFIG_H
#define CORE_CONFIG_H

typedef struct {
    const char *environment;
    int port;
} CoreConfig;

/* Load configuration from environment variables:
 * CORE_ENV  -> environment (default: "development")
 * CORE_PORT -> port        (default: 3000)
 */
CoreConfig core_config_load(void);

#endif /* CORE_CONFIG_H */

