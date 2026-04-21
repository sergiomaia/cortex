#include "db_migrate.h"
#include "db_create.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "db_connection.h"
#include "db_paths.h"
#include "sql_migrate.h"

static int ensure_dir(const char *path) {
    if (!path || path[0] == '\0') return -1;

    if (mkdir(path, 0755) == -1) {
        if (errno == EEXIST) return 0;
        return -1;
    }
    return 0;
}

static int file_exists(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) return 0;
    fclose(f);
    return 1;
}

static int path_is_json_storage(const char *path) {
    static const char suf[] = ".json";
    size_t len;
    size_t slen;

    if (!path) {
        return 0;
    }
    len = strlen(path);
    slen = sizeof(suf) - 1;
    if (len < slen) {
        return 0;
    }
    return strcmp(path + (len - slen), suf) == 0;
}

static int cmp_ints(const void *a, const void *b) {
    int ai = *(const int *)a;
    int bi = *(const int *)b;
    return (ai > bi) - (ai < bi);
}

/* Load executed versions from a JSON-like array by scanning numbers. */
static int load_executed_versions_json(const char *storage_path, int *out_versions, int out_cap) {
    FILE *f;
    char buf[2048];
    size_t nread;
    int count = 0;
    long value = 0;
    int in_number = 0;
    int sign = 1;

    if (!storage_path || !out_versions || out_cap <= 0) return -1;

    f = fopen(storage_path, "r");
    if (!f) return -1;

    while ((nread = fread(buf, 1, sizeof(buf), f)) > 0) {
        size_t i;
        for (i = 0; i < nread; ++i) {
            char c = buf[i];

            if (!in_number) {
                if (c == '-') {
                    in_number = 1;
                    sign = -1;
                    value = 0;
                } else if (isdigit((unsigned char)c)) {
                    in_number = 1;
                    sign = 1;
                    value = (long)(c - '0');
                }
            } else {
                if (isdigit((unsigned char)c)) {
                    value = value * 10 + (long)(c - '0');
                } else {
                    /* Finish number. */
                    if (count < out_cap) {
                        out_versions[count++] = (int)(sign * value);
                    }
                    in_number = 0;
                    sign = 1;
                    value = 0;
                    /* Re-process current char for new number */
                    if (c == '-') {
                        in_number = 1;
                        sign = -1;
                        value = 0;
                    } else if (isdigit((unsigned char)c)) {
                        in_number = 1;
                        sign = 1;
                        value = (long)(c - '0');
                    }
                }
            }
        }
    }

    /* Flush trailing number if present. */
    if (in_number && count < out_cap) {
        out_versions[count++] = (int)(sign * value);
    }

    fclose(f);
    return count;
}

static int persist_executed_versions_json(const char *storage_path, const ActiveMigrationRegistry *registry) {
    FILE *f;
    int i;

    if (!storage_path || !registry) return -1;

    f = fopen(storage_path, "w");
    if (!f) return -1;

    /* Ensure sorted output for stable tests/debugging. */
    if (registry->executed_count > 1) {
        int *tmp = (int *)malloc((size_t)registry->executed_count * sizeof(int));
        if (!tmp) {
            fclose(f);
            return -1;
        }
        for (i = 0; i < registry->executed_count; ++i) {
            tmp[i] = registry->executed_versions[i];
        }
        qsort(tmp, (size_t)registry->executed_count, sizeof(int), cmp_ints);
        fputc('[', f);
        for (i = 0; i < registry->executed_count; ++i) {
            if (i > 0) fputc(',', f);
            fprintf(f, "%d", tmp[i]);
        }
        fputc(']', f);
        fputc('\n', f);
        free(tmp);
    } else {
        fputc('[', f);
        for (i = 0; i < registry->executed_count; ++i) {
            if (i > 0) fputc(',', f);
            fprintf(f, "%d", registry->executed_versions[i]);
        }
        fputc(']', f);
        fputc('\n', f);
    }

    if (fclose(f) != 0) return -1;
    return 0;
}

static int sqlite_ensure_schema(DbConnection *conn) {
    static const char *ddl =
        "CREATE TABLE IF NOT EXISTS schema_migrations (\n"
        "  version INTEGER PRIMARY KEY NOT NULL\n"
        ");";
    return db_connection_exec(conn, ddl);
}

static int sqlite_schema_migrations_exists(DbConnection *conn) {
    DbStatement *st = NULL;
    int step;
    int exists = 0;

    if (!conn) {
        return -1;
    }

    if (db_connection_prepare(
            conn,
            "SELECT 1 FROM sqlite_master WHERE type = 'table' AND name = 'schema_migrations'",
            &st) != 0) {
        return -1;
    }

    step = db_statement_step(st);
    if (step == 1) {
        exists = 1;
    } else if (step < 0) {
        db_statement_finalize(st);
        return -1;
    }

    db_statement_finalize(st);
    return exists;
}

static int sqlite_load_executed(DbConnection *conn, int *out_versions, int out_cap) {
    DbStatement *st = NULL;
    int count = 0;

    if (!conn || !out_versions || out_cap <= 0) {
        return -1;
    }

    if (db_connection_prepare(conn, "SELECT version FROM schema_migrations ORDER BY version ASC", &st) != 0) {
        return -1;
    }

    while (1) {
        int step = db_statement_step(st);
        if (step == 0) {
            break;
        }
        if (step < 0) {
            db_statement_finalize(st);
            return -1;
        }
        if (count < out_cap) {
            out_versions[count++] = db_statement_column_int(st, 0);
        }
    }

    db_statement_finalize(st);
    return count;
}

static int sqlite_sync_executed(DbConnection *conn, const ActiveMigrationRegistry *registry) {
    int i;

    if (!conn || !registry) {
        return -1;
    }

    if (db_connection_exec(conn, "DELETE FROM schema_migrations") != 0) {
        return -1;
    }

    for (i = 0; i < registry->executed_count; ++i) {
        char sql[128];
        if (snprintf(sql, sizeof(sql), "INSERT INTO schema_migrations (version) VALUES (%d)", registry->executed_versions[i]) >= (int)sizeof(sql)) {
            return -1;
        }
        if (db_connection_exec(conn, sql) != 0) {
            return -1;
        }
    }

    return 0;
}

static int db_migrate_sqlite_has_pending_at_path(const char *storage_path, const DbMigration *migrations, int migration_count, int *out_has_pending) {
    ActiveMigrationRegistry registry;
    int executed_versions[256];
    int executed_loaded = 0;
    int i;
    int schema_exists;
    int sql_pending = 0;
    int has_pending = 0;
    DbConnection *conn = NULL;

    if (!storage_path || storage_path[0] == '\0' || !out_has_pending) {
        return -1;
    }
    if (!migrations && migration_count != 0) return -1;
    if (migration_count < 0 || migration_count > 1024) return -1;

    *out_has_pending = 0;

    if (!file_exists(storage_path)) {
        *out_has_pending = 1;
        return 0;
    }

    if (db_connection_open(storage_path, &conn) != 0) {
        return -1;
    }

    active_migration_registry_init(&registry);
    for (i = 0; i < migration_count; ++i) {
        if (active_migration_register(&registry, migrations[i].version, migrations[i].name, migrations[i].up) != 0) {
            active_migration_registry_free(&registry);
            db_connection_close(conn);
            return -1;
        }
    }

    schema_exists = sqlite_schema_migrations_exists(conn);
    if (schema_exists < 0) {
        active_migration_registry_free(&registry);
        db_connection_close(conn);
        return -1;
    }

    if (schema_exists) {
        executed_loaded = sqlite_load_executed(conn, executed_versions, (int)(sizeof(executed_versions) / sizeof(executed_versions[0])));
        if (executed_loaded < 0) {
            active_migration_registry_free(&registry);
            db_connection_close(conn);
            return -1;
        }
        for (i = 0; i < executed_loaded; ++i) {
            if (active_migration_mark_executed(&registry, executed_versions[i]) != 0) {
                active_migration_registry_free(&registry);
                db_connection_close(conn);
                return -1;
            }
        }
    }

    for (i = 0; i < registry.count; ++i) {
        if (!active_migration_is_executed(&registry, registry.migrations[i].version)) {
            has_pending = 1;
            break;
        }
    }

    if (db_sql_migrations_has_pending(conn, &sql_pending) != 0) {
        active_migration_registry_free(&registry);
        db_connection_close(conn);
        return -1;
    }
    if (sql_pending) {
        has_pending = 1;
    }

    active_migration_registry_free(&registry);
    db_connection_close(conn);

    *out_has_pending = has_pending;
    return 0;
}

static int db_migrate_sqlite_at_path(const char *storage_path, const DbMigration *migrations, int migration_count) {
    ActiveMigrationRegistry registry;
    int executed_versions[256];
    int executed_loaded;
    int rc;
    int i;
    DbConnection *conn = NULL;

    if (!storage_path || storage_path[0] == '\0') {
        return -1;
    }
    if (!migrations && migration_count != 0) return -1;
    if (migration_count < 0 || migration_count > 1024) return -1;

    if (ensure_dir("db") != 0) return -1;

    if (!file_exists(storage_path)) {
        if (db_create(storage_path) != 0) return -1;
    }

    if (db_connection_open(storage_path, &conn) != 0) {
        return -1;
    }

    if (sqlite_ensure_schema(conn) != 0) {
        db_connection_close(conn);
        return -1;
    }

    active_migration_registry_init(&registry);

    executed_loaded = sqlite_load_executed(conn, executed_versions, (int)(sizeof(executed_versions) / sizeof(executed_versions[0])));
    if (executed_loaded < 0) {
        executed_loaded = 0;
    }

    for (i = 0; i < executed_loaded; ++i) {
        (void)active_migration_mark_executed(&registry, executed_versions[i]);
    }

    for (i = 0; i < migration_count; ++i) {
        if (active_migration_register(&registry, migrations[i].version, migrations[i].name, migrations[i].up) != 0) {
            active_migration_registry_free(&registry);
            db_connection_close(conn);
            return -1;
        }
    }

    rc = active_migration_run_pending(&registry);

    if (sqlite_sync_executed(conn, &registry) != 0) {
        active_migration_registry_free(&registry);
        db_connection_close(conn);
        return -1;
    }

    active_migration_registry_free(&registry);

    if (db_sql_migrations_run(conn) != 0) {
        db_connection_close(conn);
        return -1;
    }

    if (db_schema_write_dump(conn, "db/schema.sql") != 0) {
        fprintf(stderr, "db: warning: could not write db/schema.sql\n");
    }

    db_connection_close(conn);

    /* Treat rc==1 ("nothing pending") as success. */
    if (rc == 1) return 0;
    return rc;
}

static int db_migrate_json_at_path(const char *storage_path, const DbMigration *migrations, int migration_count) {
    ActiveMigrationRegistry registry;
    int executed_versions[256];
    int executed_loaded;
    int rc;
    int i;
    const char *path = storage_path ? storage_path : "db/storage.json";

    if (!migrations && migration_count != 0) return -1;
    if (migration_count < 0 || migration_count > 1024) return -1;

    if (ensure_dir("db") != 0) return -1;

    if (!file_exists(path)) {
        if (db_create(path) != 0) return -1;
    }

    active_migration_registry_init(&registry);

    executed_loaded = load_executed_versions_json(path, executed_versions, (int)(sizeof(executed_versions) / sizeof(executed_versions[0])));
    if (executed_loaded < 0) {
        /* If storage is unreadable, treat as empty. */
        executed_loaded = 0;
    }

    for (i = 0; i < executed_loaded; ++i) {
        (void)active_migration_mark_executed(&registry, executed_versions[i]);
    }

    for (i = 0; i < migration_count; ++i) {
        if (active_migration_register(&registry, migrations[i].version, migrations[i].name, migrations[i].up) != 0) {
            active_migration_registry_free(&registry);
            return -1;
        }
    }

    rc = active_migration_run_pending(&registry);

    /* Persist executed versions regardless of whether anything was pending. */
    if (persist_executed_versions_json(path, &registry) != 0) {
        active_migration_registry_free(&registry);
        return -1;
    }

    active_migration_registry_free(&registry);

    /* Treat rc==1 ("nothing pending") as success. */
    if (rc == 1) return 0;
    return rc;
}

int db_migrate_has_pending(const char *storage_path, const DbMigration *migrations, int migration_count, int *out_has_pending) {
    char buf[512];
    const char *path = storage_path;

    if (!out_has_pending) {
        return -1;
    }

    if (!path || path[0] == '\0') {
        if (db_path_for_environment(NULL, buf, sizeof(buf)) != 0) {
            return -1;
        }
        path = buf;
    }

    if (path_is_json_storage(path)) {
        /* JSON storage is legacy and only used in tests; keep old behavior. */
        *out_has_pending = 0;
        return 0;
    }
    return db_migrate_sqlite_has_pending_at_path(path, migrations, migration_count, out_has_pending);
}

int db_migrate(const char *storage_path, const DbMigration *migrations, int migration_count) {
    char buf[512];
    const char *path = storage_path;

    if (!path || path[0] == '\0') {
        if (db_path_for_environment(NULL, buf, sizeof(buf)) != 0) {
            return -1;
        }
        path = buf;
    }

    if (path_is_json_storage(path)) {
        return db_migrate_json_at_path(path, migrations, migration_count);
    }
    return db_migrate_sqlite_at_path(path, migrations, migration_count);
}

/* Default migrations for the scaffold: versioned no-op migrations. */
static int migration_1_up(void) {
    return 0;
}

static int migration_2_up(void) {
    return 0;
}

int db_migrate_default(const char *storage_path) {
    DbMigration migrations[2];
    char buf[512];
    const char *path = storage_path;

    migrations[0].version = 1;
    migrations[0].name = "init";
    migrations[0].up = migration_1_up;

    migrations[1].version = 2;
    migrations[1].name = "second";
    migrations[1].up = migration_2_up;

    if (!path || path[0] == '\0') {
        if (db_path_for_environment(NULL, buf, sizeof(buf)) != 0) {
            return -1;
        }
        path = buf;
    }

    return db_migrate(path, migrations, 2);
}

int db_migrate_default_has_pending(const char *storage_path, int *out_has_pending) {
    DbMigration migrations[2];
    char buf[512];
    const char *path = storage_path;

    migrations[0].version = 1;
    migrations[0].name = "init";
    migrations[0].up = migration_1_up;

    migrations[1].version = 2;
    migrations[1].name = "second";
    migrations[1].up = migration_2_up;

    if (!out_has_pending) {
        return -1;
    }

    if (!path || path[0] == '\0') {
        if (db_path_for_environment(NULL, buf, sizeof(buf)) != 0) {
            return -1;
        }
        path = buf;
    }

    return db_migrate_has_pending(path, migrations, 2, out_has_pending);
}
