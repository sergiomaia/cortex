#include "sql_migrate.h"

#include <glob.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

static int ensure_sql_migration_table(DbConnection *conn) {
    static const char *ddl =
        "CREATE TABLE IF NOT EXISTS cortex_sql_migrations (\n"
        "  name TEXT PRIMARY KEY NOT NULL\n"
        ");";
    return db_connection_exec(conn, ddl);
}

static int sql_migration_table_exists(DbConnection *conn) {
    DbStatement *st = NULL;
    int step;
    int exists = 0;

    if (db_connection_prepare(
            conn,
            "SELECT 1 FROM sqlite_master WHERE type = 'table' AND name = 'cortex_sql_migrations'",
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

static int sql_migration_executed(DbConnection *conn, const char *name) {
    DbStatement *st = NULL;
    int step;
    int found = 0;

    if (db_connection_prepare(conn, "SELECT 1 FROM cortex_sql_migrations WHERE name = ?", &st) != 0) {
        return -1;
    }
    if (db_statement_bind_text(st, 1, name) != 0) {
        db_statement_finalize(st);
        return -1;
    }
    step = db_statement_step(st);
    if (step == 1) {
        found = 1;
    } else if (step < 0) {
        db_statement_finalize(st);
        return -1;
    }
    db_statement_finalize(st);
    return found;
}

static int sql_migration_mark(DbConnection *conn, const char *name) {
    DbStatement *st = NULL;
    int step;

    if (db_connection_prepare(conn, "INSERT INTO cortex_sql_migrations (name) VALUES (?)", &st) != 0) {
        return -1;
    }
    if (db_statement_bind_text(st, 1, name) != 0) {
        db_statement_finalize(st);
        return -1;
    }
    step = db_statement_step(st);
    db_statement_finalize(st);
    if (step != 0) {
        return -1;
    }
    return 0;
}

static char *read_file_all(const char *path, size_t *out_len) {
    FILE *f;
    long len;
    char *buf;

    f = fopen(path, "rb");
    if (!f) {
        return NULL;
    }
    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        return NULL;
    }
    len = ftell(f);
    if (len < 0) {
        fclose(f);
        return NULL;
    }
    if (fseek(f, 0, SEEK_SET) != 0) {
        fclose(f);
        return NULL;
    }
    buf = (char *)malloc((size_t)len + 1);
    if (!buf) {
        fclose(f);
        return NULL;
    }
    if (len > 0 && fread(buf, 1, (size_t)len, f) != (size_t)len) {
        free(buf);
        fclose(f);
        return NULL;
    }
    buf[len] = '\0';
    fclose(f);
    if (out_len) {
        *out_len = (size_t)len;
    }
    return buf;
}

static int cmp_cstr(const void *a, const void *b) {
    const char *const *sa = (const char *const *)a;
    const char *const *sb = (const char *const *)b;
    return strcmp(*sa, *sb);
}

int db_sql_migrations_has_pending(DbConnection *conn, int *out_has_pending) {
    glob_t g;
    size_t i;
    int rc;
    const char **names = NULL;
    size_t count = 0;
    int has_pending = 0;
    int table_exists = 0;
    int ret = 0;

    if (!conn || !out_has_pending) {
        return -1;
    }
    *out_has_pending = 0;

    rc = glob("db/migrate/*.sql", 0, NULL, &g);
    if (rc == GLOB_NOMATCH) {
        globfree(&g);
        return 0;
    }
    if (rc != 0) {
        fprintf(stderr, "db: glob db/migrate/*.sql failed\n");
        globfree(&g);
        return -1;
    }

    count = g.gl_pathc;
    if (count == 0) {
        globfree(&g);
        return 0;
    }

    table_exists = sql_migration_table_exists(conn);
    if (table_exists < 0) {
        globfree(&g);
        return -1;
    }
    if (!table_exists) {
        *out_has_pending = 1;
        globfree(&g);
        return 0;
    }

    names = (const char **)malloc(count * sizeof(const char *));
    if (!names) {
        globfree(&g);
        return -1;
    }
    for (i = 0; i < count; ++i) {
        names[i] = g.gl_pathv[i];
    }
    qsort(names, count, sizeof(names[0]), cmp_cstr);

    for (i = 0; i < count; ++i) {
        const char *path = names[i];
        const char *base = strrchr(path, '/');
        int ex;

        base = base ? base + 1 : path;
        ex = sql_migration_executed(conn, base);
        if (ex < 0) {
            ret = -1;
            break;
        }
        if (!ex) {
            has_pending = 1;
            break;
        }
    }

    free(names);
    globfree(&g);

    if (ret != 0) {
        return ret;
    }
    *out_has_pending = has_pending;
    return 0;
}

int db_sql_migrations_run(DbConnection *conn) {
    glob_t g;
    size_t i;
    int rc;
    const char **names = NULL;
    size_t count = 0;
    int ret = 0;

    if (!conn) {
        return -1;
    }

    if (ensure_sql_migration_table(conn) != 0) {
        fprintf(stderr, "db: failed to ensure cortex_sql_migrations table\n");
        return -1;
    }

    rc = glob("db/migrate/*.sql", 0, NULL, &g);
    if (rc == GLOB_NOMATCH) {
        globfree(&g);
        return 0;
    }
    if (rc != 0) {
        fprintf(stderr, "db: glob db/migrate/*.sql failed\n");
        globfree(&g);
        return -1;
    }

    count = g.gl_pathc;
    if (count == 0) {
        globfree(&g);
        return 0;
    }

    names = (const char **)malloc(count * sizeof(const char *));
    if (!names) {
        globfree(&g);
        return -1;
    }
    for (i = 0; i < count; ++i) {
        names[i] = g.gl_pathv[i];
    }
    qsort(names, count, sizeof(names[0]), cmp_cstr);

    for (i = 0; i < count; ++i) {
        const char *path = names[i];
        const char *base;
        int ex;

        base = strrchr(path, '/');
        base = base ? base + 1 : path;

        ex = sql_migration_executed(conn, base);
        if (ex < 0) {
            ret = -1;
            break;
        }
        if (ex) {
            continue;
        }

        {
            char *sql = read_file_all(path, NULL);
            if (!sql) {
                fprintf(stderr, "db: failed to read migration %s\n", path);
                ret = -1;
                break;
            }
            if (db_connection_exec(conn, sql) != 0) {
                fprintf(stderr, "db: SQL migration failed: %s\n", path);
                free(sql);
                ret = -1;
                break;
            }
            free(sql);
        }

        if (sql_migration_mark(conn, base) != 0) {
            fprintf(stderr, "db: failed to record migration %s\n", base);
            ret = -1;
            break;
        }
    }

    free(names);
    globfree(&g);
    return ret;
}

int db_schema_write_dump(DbConnection *conn, const char *schema_path) {
    DbStatement *st = NULL;
    FILE *out;
    int step;
    int first = 1;

    if (!conn || !schema_path) {
        return -1;
    }

    if (db_connection_prepare(
            conn,
            "SELECT sql FROM sqlite_master WHERE sql IS NOT NULL AND name NOT LIKE 'sqlite_%%' "
            "ORDER BY CASE type WHEN 'table' THEN 0 WHEN 'index' THEN 1 ELSE 2 END, name",
            &st) != 0) {
        return -1;
    }

    out = fopen(schema_path, "w");
    if (!out) {
        db_statement_finalize(st);
        fprintf(stderr, "db: cannot write %s\n", schema_path);
        return -1;
    }

    fputs("-- Cortex schema dump (generated by db:migrate)\n", out);
    fputs("PRAGMA foreign_keys=OFF;\n", out);

    for (;;) {
        step = db_statement_step(st);
        if (step == 0) {
            break;
        }
        if (step < 0) {
            fclose(out);
            db_statement_finalize(st);
            return -1;
        }
        {
            const char *sql = db_statement_column_text(st, 0);
            if (sql && sql[0]) {
                if (!first) {
                    fputc('\n', out);
                }
                first = 0;
                fputs(sql, out);
                fputs(";\n", out);
            }
        }
    }

    db_statement_finalize(st);
    if (fclose(out) != 0) {
        return -1;
    }
    return 0;
}
