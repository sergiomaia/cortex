#include <stdlib.h>

#include "../../core/active_migration.h"
#include "../test_assert.h"

static int migration_calls[3];
static int execution_order[3];
static int execution_index;

static int migration_one(void) {
    migration_calls[0] += 1;
    execution_order[execution_index++] = 1;
    return 0;
}

static int migration_two(void) {
    migration_calls[1] += 1;
    execution_order[execution_index++] = 2;
    return 0;
}

static int migration_three(void) {
    migration_calls[2] += 1;
    execution_order[execution_index++] = 3;
    return 0;
}

void test_active_migration_register_and_execute_in_order(void) {
    ActiveMigrationRegistry registry;
    int result;

    migration_calls[0] = 0;
    migration_calls[1] = 0;
    migration_calls[2] = 0;
    execution_index = 0;

    active_migration_registry_init(&registry);

    /* Register out of order to ensure execution respects version ordering. */
    ASSERT_EQ(active_migration_register(&registry, 2, "second", migration_two), 0);
    ASSERT_EQ(active_migration_register(&registry, 1, "first", migration_one), 0);
    ASSERT_EQ(active_migration_register(&registry, 3, "third", migration_three), 0);

    result = active_migration_run_pending(&registry);
    ASSERT_EQ(result, 0);

    /* All migrations should have been executed exactly once. */
    ASSERT_EQ(migration_calls[0], 1);
    ASSERT_EQ(migration_calls[1], 1);
    ASSERT_EQ(migration_calls[2], 1);

    /* Ensure migrations execute in ascending version order. */
    ASSERT_EQ(execution_order[0], 1);
    ASSERT_EQ(execution_order[1], 2);
    ASSERT_EQ(execution_order[2], 3);

    /* All versions should be marked as executed. */
    ASSERT_EQ(active_migration_is_executed(&registry, 1), 1);
    ASSERT_EQ(active_migration_is_executed(&registry, 2), 1);
    ASSERT_EQ(active_migration_is_executed(&registry, 3), 1);

    /* Verify in-memory executed-migrations tracking list. */
    ASSERT_EQ(registry.executed_count, 3);
    ASSERT_EQ(registry.executed_versions[0], 1);
    ASSERT_EQ(registry.executed_versions[1], 2);
    ASSERT_EQ(registry.executed_versions[2], 3);

    active_migration_registry_free(&registry);
}

void test_active_migration_run_only_pending(void) {
    ActiveMigrationRegistry registry;
    int result;
    int executed_count_before;

    migration_calls[0] = 0;
    migration_calls[1] = 0;
    migration_calls[2] = 0;
    execution_index = 0;

    active_migration_registry_init(&registry);

    ASSERT_EQ(active_migration_register(&registry, 1, "first", migration_one), 0);
    ASSERT_EQ(active_migration_register(&registry, 2, "second", migration_two), 0);

    /* First run executes all pending migrations. */
    result = active_migration_run_pending(&registry);
    ASSERT_EQ(result, 0);

    ASSERT_EQ(migration_calls[0], 1);
    ASSERT_EQ(migration_calls[1], 1);
    ASSERT_EQ(migration_calls[2], 0);

    /* Verify executed-migrations list after first run. */
    ASSERT_EQ(registry.executed_count, 2);
    ASSERT_EQ(registry.executed_versions[0], 1);
    ASSERT_EQ(registry.executed_versions[1], 2);

    /* Second run should not re-execute already executed migrations. */
    executed_count_before = registry.executed_count;
    result = active_migration_run_pending(&registry);
    ASSERT_EQ(result, 1); /* nothing pending */

    ASSERT_EQ(migration_calls[0], 1);
    ASSERT_EQ(migration_calls[1], 1);
    ASSERT_EQ(migration_calls[2], 0);
    ASSERT_EQ(registry.executed_count, executed_count_before);
    ASSERT_EQ(registry.executed_versions[0], 1);
    ASSERT_EQ(registry.executed_versions[1], 2);

    /* Register a new migration; only it should be executed. */
    ASSERT_EQ(active_migration_register(&registry, 3, "third", migration_three), 0);

    result = active_migration_run_pending(&registry);
    ASSERT_EQ(result, 0);

    ASSERT_EQ(migration_calls[0], 1);
    ASSERT_EQ(migration_calls[1], 1);
    ASSERT_EQ(migration_calls[2], 1);

    /* Verify executed-migrations list after running new pending migration. */
    ASSERT_EQ(registry.executed_count, 3);
    ASSERT_EQ(registry.executed_versions[0], 1);
    ASSERT_EQ(registry.executed_versions[1], 2);
    ASSERT_EQ(registry.executed_versions[2], 3);

    active_migration_registry_free(&registry);
}

