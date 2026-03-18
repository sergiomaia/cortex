#include <stdlib.h>

#include "active_migration.h"

static int ensure_migration_capacity(ActiveMigrationRegistry *registry) {
    int new_capacity;
    ActiveMigration *new_items;

    if (registry->capacity == 0) {
        new_capacity = 4;
    } else if (registry->count >= registry->capacity) {
        new_capacity = registry->capacity * 2;
    } else {
        return 0;
    }

    new_items = (ActiveMigration *)realloc(registry->migrations, new_capacity * sizeof(ActiveMigration));
    if (!new_items) {
        return -1;
    }

    registry->migrations = new_items;
    registry->capacity = new_capacity;
    return 0;
}

static int ensure_executed_capacity(ActiveMigrationRegistry *registry) {
    int new_capacity;
    int *new_items;

    if (registry->executed_capacity == 0) {
        new_capacity = 4;
    } else if (registry->executed_count >= registry->executed_capacity) {
        new_capacity = registry->executed_capacity * 2;
    } else {
        return 0;
    }

    new_items = (int *)realloc(registry->executed_versions, new_capacity * sizeof(int));
    if (!new_items) {
        return -1;
    }

    registry->executed_versions = new_items;
    registry->executed_capacity = new_capacity;
    return 0;
}

void active_migration_registry_init(ActiveMigrationRegistry *registry) {
    if (!registry) {
        return;
    }

    registry->migrations = NULL;
    registry->count = 0;
    registry->capacity = 0;

    registry->executed_versions = NULL;
    registry->executed_count = 0;
    registry->executed_capacity = 0;
}

int active_migration_register(ActiveMigrationRegistry *registry, int version, const char *name, ActiveMigrationFn up) {
    ActiveMigration *m;

    if (!registry || !name || !up || version <= 0) {
        return -1;
    }

    if (ensure_migration_capacity(registry) != 0) {
        return -1;
    }

    m = &registry->migrations[registry->count];
    m->version = version;
    m->name = name;
    m->up = up;
    registry->count += 1;

    return 0;
}

int active_migration_is_executed(ActiveMigrationRegistry *registry, int version) {
    int i;

    if (!registry || version <= 0) {
        return 0;
    }

    for (i = 0; i < registry->executed_count; ++i) {
        if (registry->executed_versions[i] == version) {
            return 1;
        }
    }

    return 0;
}

static int mark_executed(ActiveMigrationRegistry *registry, int version) {
    if (ensure_executed_capacity(registry) != 0) {
        return -1;
    }

    registry->executed_versions[registry->executed_count++] = version;
    return 0;
}

int active_migration_run_pending(ActiveMigrationRegistry *registry) {
    int executed_any = 0;

    if (!registry) {
        return -1;
    }

    while (1) {
        int i;
        int next_index = -1;
        int next_version = 0;

        for (i = 0; i < registry->count; ++i) {
            ActiveMigration *m = &registry->migrations[i];

            if (active_migration_is_executed(registry, m->version)) {
                continue;
            }

            if (next_index == -1 || m->version < next_version) {
                next_index = i;
                next_version = m->version;
            }
        }

        if (next_index == -1) {
            break;
        }

        if (registry->migrations[next_index].up() != 0) {
            return -1;
        }

        if (mark_executed(registry, registry->migrations[next_index].version) != 0) {
            return -1;
        }

        executed_any = 1;
    }

    return executed_any ? 0 : 1;
}

void active_migration_registry_free(ActiveMigrationRegistry *registry) {
    if (!registry) {
        return;
    }

    free(registry->migrations);
    registry->migrations = NULL;
    registry->count = 0;
    registry->capacity = 0;

    free(registry->executed_versions);
    registry->executed_versions = NULL;
    registry->executed_count = 0;
    registry->executed_capacity = 0;
}

