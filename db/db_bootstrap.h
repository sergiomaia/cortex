#ifndef DB_BOOTSTRAP_H
#define DB_BOOTSTRAP_H

#include "db_connection.h"

/* Open the default SQLite database for the current environment and keep
 * one connection for the process lifetime. */
int cortex_db_init(void);
void cortex_db_shutdown(void);

DbConnection *cortex_db_connection(void);

/* Convenience for migrations and app code (uses the active connection). */
int cortex_db_exec(const char *sql);

/* Open DB (if needed) and run default migrations — safe to call on every boot. */
int cortex_db_bootstrap(void);

#endif /* DB_BOOTSTRAP_H */
