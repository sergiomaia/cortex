#ifndef DB_BOOTSTRAP_H
#define DB_BOOTSTRAP_H

#include "db_connection.h"

/*
 * Returns 1 when the process environment selects PostgreSQL
 * (DATABASE_URL starting with postgres / postgresql, or DB_ADAPTER=postgresql).
 * Always 0 when the runtime does not request PostgreSQL.
 */
int cortex_db_env_wants_postgresql(void);

/* Open the default database for the current environment and keep
 * one connection handle for the process lifetime (via the pool). SQLite is the default. */
int cortex_db_init(void);
void cortex_db_shutdown(void);

DbConnection *cortex_db_connection(void);

/* Convenience for migrations and app code (uses the active connection). */
int cortex_db_exec(const char *sql);

/*
 * Prepare secrets, reconcile migrations for the current environment, then open the pool.
 *
 * When CORE_ENV is production: pending migrations are not applied; a Pulse warning is
 * emitted and startup continues after cortex_db_init().
 *
 * When CORE_ENV is unset, development, test, or any non-production value: migrations
 * are applied automatically (SQLite via db_migrate_default, PostgreSQL via
 * db_migrate_default_on_connection on a probe connection). Failure returns -1.
 */
int cortex_db_bootstrap(void);

#endif /* DB_BOOTSTRAP_H */
