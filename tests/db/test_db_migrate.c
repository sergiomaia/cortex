#include "../test_assert.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../../db/db_create.h"
#include "../../db/db_migrate.h"

static int file_exists(const char *path) {
    FILE *f = fopen(path, "r");
    if (f == NULL) return 0;
    fclose(f);
    return 1;
}

static int file_contains(const char *path, const char *substring) {
    FILE *f;
    char buf[512];
    int found = 0;

    if (!substring || substring[0] == '\0') return 1;

    f = fopen(path, "r");
    if (!f) return 0;

    while (fgets(buf, (int)sizeof(buf), f) != NULL) {
        if (strstr(buf, substring) != NULL) {
            found = 1;
            break;
        }
    }

    fclose(f);
    return found;
}

static void remove_if_exists(const char *path) {
    if (file_exists(path)) {
        remove(path);
    }
}

static int write_file_from_string(const char *path, const char *content) {
    FILE *f;
    size_t len;
    size_t written;

    if (!path || !content) return -1;
    f = fopen(path, "w");
    if (!f) return -1;

    len = strlen(content);
    written = fwrite(content, 1, len, f);
    if (fclose(f) != 0) return -1;
    if (written != len) return -1;
    return 0;
}

static int db_file_read_all(const char *path, char *out, size_t out_size) {
    FILE *f;
    size_t nread;

    if (!path || !out || out_size == 0) return -1;
    out[0] = '\0';

    f = fopen(path, "r");
    if (!f) return -1;

    nread = fread(out, 1, out_size - 1, f);
    out[nread] = '\0';

    fclose(f);
    return 0;
}

static int calls_v1 = 0;
static int calls_v2 = 0;

static int migration_1_up(void) {
    calls_v1 += 1;
    return 0;
}

static int migration_2_up(void) {
    calls_v2 += 1;
    return 0;
}

void test_db_migrate_runs_pending_and_tracks_executed(void) {
    const char *storage = "db/test_storage_migrate.json";
    DbMigration migrations[2];
    char content_before[256];
    char content_after[256];

    /* Cleanup. */
    remove_if_exists(storage);

    ASSERT_EQ(db_create(storage), 0);

    calls_v1 = 0;
    calls_v2 = 0;

    migrations[0].version = 1;
    migrations[0].name = "m1";
    migrations[0].up = migration_1_up;

    migrations[1].version = 2;
    migrations[1].name = "m2";
    migrations[1].up = migration_2_up;

    ASSERT_EQ(db_migrate(storage, migrations, 2), 0);
    ASSERT_EQ(calls_v1, 1);
    ASSERT_EQ(calls_v2, 1);

    ASSERT_TRUE(file_contains(storage, "1"));
    ASSERT_TRUE(file_contains(storage, "2"));

    ASSERT_EQ(db_file_read_all(storage, content_before, sizeof(content_before)), 0);

    /* Second run: should not re-execute. */
    ASSERT_EQ(db_migrate(storage, migrations, 2), 0);
    ASSERT_EQ(calls_v1, 1);
    ASSERT_EQ(calls_v2, 1);

    ASSERT_EQ(db_file_read_all(storage, content_after, sizeof(content_after)), 0);
    ASSERT_STR_EQ(content_before, content_after);

    /* Cleanup. */
    remove_if_exists(storage);
}

void test_db_migrate_default_has_pending_tracks_sql_files(void) {
    const char *storage = "db/test_storage_pending.sqlite3";
    const char *migration_path = "db/migrate/20990101010101_pending_spec.sql";
    int has_pending = 0;

    remove_if_exists(storage);
    remove_if_exists(migration_path);

    ASSERT_EQ(db_create(storage), 0);
    ASSERT_EQ(write_file_from_string(migration_path, "CREATE TABLE pending_specs (id INTEGER PRIMARY KEY);\n"), 0);

    ASSERT_EQ(db_migrate_default_has_pending(storage, &has_pending), 0);
    ASSERT_EQ(has_pending, 1);

    ASSERT_EQ(db_migrate_default(storage), 0);
    ASSERT_EQ(db_migrate_default_has_pending(storage, &has_pending), 0);
    ASSERT_EQ(has_pending, 0);

    remove_if_exists(migration_path);
    remove_if_exists(storage);
}

