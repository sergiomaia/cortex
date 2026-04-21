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

/* Open DB only when migrations are up to date for this environment.
 * Returns -1 and prints guidance when pending migrations are detected. */
int cortex_db_bootstrap(void);

#endif /* DB_BOOTSTRAP_H */
