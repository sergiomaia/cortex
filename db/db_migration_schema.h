#ifndef DB_MIGRATION_SCHEMA_H
#define DB_MIGRATION_SCHEMA_H

#include "db_connection.h"

/*
 * Tracks applied SQL file migrations in table schema_migrations.
 * Coexists with legacy C callback versions stored as zero-padded 14-digit strings
 * below the sentinel '19900101000000' (lexicographic sort).
 */

int db_migration_ensure_table(DbConnection *conn);

/* out_applied: 1 if row exists, 0 if not, -1 on DB error */
int db_migration_is_applied(DbConnection *conn, const char *version, int *out_applied);

int db_migration_mark_applied(DbConnection *conn, const char *version, const char *name);

int db_migration_mark_rolled_back(DbConnection *conn, const char *version);

/* Format a C callback migration version for storage (14-digit zero-padded decimal). */
int db_migration_format_callback_version(int version, char *out, size_t out_size);

#endif /* DB_MIGRATION_SCHEMA_H */
