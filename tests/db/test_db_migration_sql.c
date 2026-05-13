#define _POSIX_C_SOURCE 200809L

#include "../test_assert.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../../db/db_connection.h"
#include "../../db/db_migration_parser.h"
#include "../../db/db_migration_runner.h"
#include "../../db/db_migration_schema.h"
#include "../../db/db_create.h"

#include "../../core/pulse.h"

static void remove_if_exists(const char *path) {
    if (access(path, F_OK) == 0) {
        remove(path);
    }
}

static int write_file(const char *path, const char *content) {
    FILE *f = fopen(path, "w");
    if (!f) {
        return -1;
    }
    if (fputs(content, f) < 0) {
        fclose(f);
        return -1;
    }
    return fclose(f) == 0 ? 0 : -1;
}

void test_migration_sql_parse_valid_sections(void) {
    const char *path = "db/migrate/20990101000000_parser_valid_test.sql";
    MigrationSQL m;

    remove_if_exists(path);
    ASSERT_EQ(write_file(path,
                         "-- preamble ignored\n"
                         "-- migrate:up\n"
                         "SELECT 1;\n"
                         "-- migrate:down\n"
                         "SELECT 2;\n"),
              0);

    ASSERT_EQ(migration_sql_parse(path, &m), 0);
    ASSERT_TRUE(m.up_sql != NULL);
    ASSERT_TRUE(strstr(m.up_sql, "SELECT 1") != NULL);
    ASSERT_TRUE(m.down_sql != NULL);
    ASSERT_TRUE(strstr(m.down_sql, "SELECT 2") != NULL);
    migration_sql_free(&m);
    remove_if_exists(path);
}

void test_migration_sql_parse_optional_down(void) {
    const char *path = "db/migrate/20990101000001_parser_no_down_test.sql";
    MigrationSQL m;

    remove_if_exists(path);
    ASSERT_EQ(write_file(path,
                         "-- migrate:up\n"
                         "SELECT 3;\n"),
              0);

    ASSERT_EQ(migration_sql_parse(path, &m), 0);
    ASSERT_TRUE(m.up_sql != NULL);
    ASSERT_TRUE(m.down_sql == NULL);
    migration_sql_free(&m);
    remove_if_exists(path);
}

void test_migration_sql_parse_rejects_missing_up(void) {
    const char *path = "db/migrate/20990101000002_parser_bad_test.sql";
    MigrationSQL m;

    remove_if_exists(path);
    ASSERT_EQ(write_file(path, "SELECT 1;\n"), 0);
    migration_sql_init(&m);
    ASSERT_TRUE(migration_sql_parse(path, &m) != 0);
    migration_sql_free(&m);
    remove_if_exists(path);
}

void test_migration_sql_parse_semicolon_in_string(void) {
    const char *path = "db/migrate/20990101000003_parser_semi_test.sql";
    MigrationSQL m;

    remove_if_exists(path);
    ASSERT_EQ(write_file(path,
                         "-- migrate:up\n"
                         "INSERT INTO t VALUES ('a;b');\n"
                         "SELECT 1;\n"
                         "-- migrate:down\n"
                         "DELETE FROM t;\n"),
              0);

    ASSERT_EQ(migration_sql_parse(path, &m), 0);
    ASSERT_TRUE(strstr(m.up_sql, "'a;b'") != NULL);
    migration_sql_free(&m);
    remove_if_exists(path);
}

void test_db_migration_scan_ignores_invalid_names(void) {
    MigrationFile *files = NULL;
    size_t n = 0;
    const char *dir = "db/migrate_test_scan";

    remove_if_exists("db/migrate_test_scan/not_a_migration.txt");
    remove_if_exists("db/migrate_test_scan/badname.sql");
    {
        int mk = mkdir(dir, 0755);
        ASSERT_TRUE(mk == 0 || (mk == -1 && errno == EEXIST));
    }
    ASSERT_EQ(write_file("db/migrate_test_scan/not_a_migration.txt", "x"), 0);
    ASSERT_EQ(write_file("db/migrate_test_scan/badname.sql", "-- migrate:up\nSELECT 1;\n"), 0);

    ASSERT_EQ(db_migration_scan_directory(dir, &files, &n), 0);
    ASSERT_EQ(n, 0u);

    free(files);
    remove_if_exists("db/migrate_test_scan/not_a_migration.txt");
    remove_if_exists("db/migrate_test_scan/badname.sql");
    rmdir("db/migrate_test_scan");
}

void test_db_migration_apply_and_rollback_sqlite(void) {
    const char *dbpath = "db/test_migration_runner.sqlite3";
    const char *iso = "db/migrate_runner_iso";
    char mpath[256];
    DbConnection *conn = NULL;
    MigrationResult r;
    const char *prev_migrate;
    int mk;

    pulse_init_from_env();

    prev_migrate = getenv("CORTEX_MIGRATE_DIR");
    mk = mkdir(iso, 0755);
    ASSERT_TRUE(mk == 0 || (mk == -1 && errno == EEXIST));

    ASSERT_TRUE(snprintf(mpath, sizeof(mpath), "%s/20990101000004_runner_up_test.sql", iso) < (int)sizeof(mpath));

    remove_if_exists(dbpath);
    remove_if_exists(mpath);

    ASSERT_EQ(setenv("CORTEX_MIGRATE_DIR", iso, 1), 0);

    ASSERT_EQ(db_create(dbpath), 0);
    ASSERT_EQ(write_file(mpath,
                         "-- migrate:up\n"
                         "CREATE TABLE rt (id INTEGER PRIMARY KEY);\n"
                         "-- migrate:down\n"
                         "DROP TABLE rt;\n"),
              0);

    ASSERT_EQ(db_connection_open(dbpath, &conn), 0);
    ASSERT_EQ(db_migration_ensure_table(conn), 0);

    r = db_migration_migrate_sql_files(conn);
    ASSERT_EQ(r.errors, 0);
    ASSERT_EQ(r.applied, 1);

    r = db_migration_rollback_sql_files(conn, 1);
    ASSERT_EQ(r.errors, 0);
    ASSERT_EQ(r.rolled_back, 1);

    db_connection_close(conn);
    remove_if_exists(mpath);
    remove_if_exists(dbpath);
    if (prev_migrate) {
        ASSERT_EQ(setenv("CORTEX_MIGRATE_DIR", prev_migrate, 1), 0);
    } else {
        ASSERT_EQ(unsetenv("CORTEX_MIGRATE_DIR"), 0);
    }
    rmdir(iso);
}

void test_db_migration_transaction_rolls_back_on_bad_sql(void) {
    const char *dbpath = "db/test_migration_fail.sqlite3";
    const char *iso = "db/migrate_runner_fail_iso";
    char mpath[256];
    DbConnection *conn = NULL;
    MigrationResult r;
    int applied = 0;
    const char *prev_migrate;
    int mk;

    pulse_init_from_env();

    prev_migrate = getenv("CORTEX_MIGRATE_DIR");
    mk = mkdir(iso, 0755);
    ASSERT_TRUE(mk == 0 || (mk == -1 && errno == EEXIST));
    ASSERT_TRUE(snprintf(mpath, sizeof(mpath), "%s/20990101000005_runner_fail_test.sql", iso) < (int)sizeof(mpath));

    remove_if_exists(dbpath);
    remove_if_exists(mpath);

    ASSERT_EQ(setenv("CORTEX_MIGRATE_DIR", iso, 1), 0);

    ASSERT_EQ(db_create(dbpath), 0);
    ASSERT_EQ(write_file(mpath,
                         "-- migrate:up\n"
                         "THIS IS NOT SQL;\n"
                         "-- migrate:down\n"
                         "SELECT 1;\n"),
              0);

    ASSERT_EQ(db_connection_open(dbpath, &conn), 0);
    r = db_migration_migrate_sql_files(conn);
    ASSERT_TRUE(r.errors != 0);

    ASSERT_EQ(db_migration_is_applied(conn, "20990101000005", &applied), 0);
    ASSERT_EQ(applied, 0);

    db_connection_close(conn);
    remove_if_exists(mpath);
    remove_if_exists(dbpath);
    if (prev_migrate) {
        ASSERT_EQ(setenv("CORTEX_MIGRATE_DIR", prev_migrate, 1), 0);
    } else {
        ASSERT_EQ(unsetenv("CORTEX_MIGRATE_DIR"), 0);
    }
    rmdir(iso);
}
