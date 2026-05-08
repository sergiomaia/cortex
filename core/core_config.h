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

/* Read values from config/database.ini [db] section. */
int core_config_get_int(const char *key, int default_value);
const char *core_config_get_string(const char *key, const char *default_value);

#endif /* CORE_CONFIG_H */

