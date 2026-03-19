#ifndef DB_MIGRATE_H
#define DB_MIGRATE_H

#include "../core/active_migration.h"

/* Single migration definition used by the migration runner. */
typedef struct {
    int version;
    const char *name;
    ActiveMigrationFn up;
} DbMigration;

/* Run pending migrations using persisted executed versions in `storage_path`.
 *
 * Storage format:
 *   - a JSON-like array of integers (e.g. `[]`, `[1,2]`)
 *   - the parser is best-effort: it just scans numbers
 */
int db_migrate(const char *storage_path, const DbMigration *migrations, int migration_count);

/* Convenience default migrations for this scaffold. */
int db_migrate_default(const char *storage_path);

#endif /* DB_MIGRATE_H */

