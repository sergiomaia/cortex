#ifndef SQL_MIGRATE_H
#define SQL_MIGRATE_H

#include "db_connection.h"

/* Run pending *.sql files in db/migrate/ (lexicographic order). Tracked in
 * table cortex_sql_migrations (name TEXT PRIMARY KEY). */
int db_sql_migrations_run(DbConnection *conn);

/* Detect if there are pending *.sql files in db/migrate/ that were not
 * recorded in cortex_sql_migrations yet.
 *
 * out_has_pending: 1 if pending migrations exist, 0 otherwise.
 */
int db_sql_migrations_has_pending(DbConnection *conn, int *out_has_pending);

/* Write sqlite_master DDL for tables/indexes/triggers to schema_path (UTF-8). */
int db_schema_write_dump(DbConnection *conn, const char *schema_path);

#endif /* SQL_MIGRATE_H */
