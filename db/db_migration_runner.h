#ifndef DB_MIGRATION_RUNNER_H
#define DB_MIGRATION_RUNNER_H

#include "db_connection.h"

#include <stdio.h>

typedef struct {
    char version[15];
    char name[256];
    char filepath[512];
} MigrationFile;

typedef struct {
    int applied;
    int rolled_back;
    int errors;
    int pending;
    int total_files;
} MigrationResult;

/*
 * Public entry used by db_migrate / CLI: apply pending timestamp SQL migrations
 * from db/migrate (or CORTEX_MIGRATE_DIR). Each migration runs in its own transaction.
 *
 * Naming note: the legacy API remains `int db_migrate(const char *storage_path, ...)`.
 * This function operates on an open connection and returns structured counts.
 */
MigrationResult db_migration_migrate_sql_files(DbConnection *conn);

MigrationResult db_migration_rollback_sql_files(DbConnection *conn, int steps);

/*
 * Prints a Rails-style status table to `out` (stdout or stderr). Counts are returned
 * in the MigrationResult (applied = up count, pending = down count).
 */
MigrationResult db_migration_status_sql_files(DbConnection *conn, FILE *out);

/*
 * Generate db/migrate/YYYYMMDDHHMMSS_<name>.sql with empty up/down sections.
 * Validates snake_case name. Returns 0 on success.
 */
int db_migration_generate(const char *name);

/* Default migration directory (override with CORTEX_MIGRATE_DIR). */
const char *db_migration_default_directory(void);

/* Scan dir for valid timestamped *.sql migrations; allocates *out_files (caller frees with free). */
int db_migration_scan_directory(const char *dir, MigrationFile **out_files, size_t *out_n);

#endif /* DB_MIGRATION_RUNNER_H */
