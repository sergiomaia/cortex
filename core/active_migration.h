#ifndef ACTIVE_MIGRATION_H
#define ACTIVE_MIGRATION_H

typedef int (*ActiveMigrationFn)(void);

typedef struct {
    int version;
    const char *name;
    ActiveMigrationFn up;
} ActiveMigration;

typedef struct {
    ActiveMigration *migrations;
    int count;
    int capacity;

    int *executed_versions;
    int executed_count;
    int executed_capacity;
} ActiveMigrationRegistry;

void active_migration_registry_init(ActiveMigrationRegistry *registry);
int active_migration_register(ActiveMigrationRegistry *registry, int version, const char *name, ActiveMigrationFn up);
int active_migration_run_pending(ActiveMigrationRegistry *registry);
int active_migration_is_executed(ActiveMigrationRegistry *registry, int version);
/* Mark a migration version as already executed in the registry. */
int active_migration_mark_executed(ActiveMigrationRegistry *registry, int version);
void active_migration_registry_free(ActiveMigrationRegistry *registry);

#endif /* ACTIVE_MIGRATION_H */

