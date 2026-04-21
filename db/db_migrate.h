#ifndef DB_MIGRATE_H
#define DB_MIGRATE_H

#include "../core/active_migration.h"

/* Single migration definition used by the migration runner. */
typedef struct {
    int version;
    const char *name;
    ActiveMigrationFn up;
} DbMigration;

/* Run pending migrations using persisted executed versions.
 *
 * If `storage_path` is NULL or empty, uses db/<CORE_ENV>.sqlite3 (default development).
 *
 * SQLite (default): versions are stored in table `schema_migrations`.
 * Legacy JSON: if the path ends with `.json`, versions are read/written as a JSON array of ints.
 */
int db_migrate(const char *storage_path, const DbMigration *migrations, int migration_count);

/* Check whether there are pending migrations (C registry and/or SQL files)
 * without applying them.
 *
 * out_has_pending: 1 if pending migrations exist, 0 otherwise.
 */
int db_migrate_has_pending(const char *storage_path, const DbMigration *migrations, int migration_count, int *out_has_pending);

/* Convenience default migrations for this scaffold. NULL path selects the default SQLite file. */
int db_migrate_default(const char *storage_path);
int db_migrate_default_has_pending(const char *storage_path, int *out_has_pending);

#endif /* DB_MIGRATE_H */

