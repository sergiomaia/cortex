#include "db_migration_runner.h"

#include "db_migration_parser.h"
#include "db_migration_schema.h"

#include "../core/pulse.h"

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

typedef struct {
    char *data;
    size_t len;
    size_t cap;
} StmtBuf;

static void stmtbuf_init(StmtBuf *b) {
    b->data = NULL;
    b->len = 0;
    b->cap = 0;
}

static void stmtbuf_free(StmtBuf *b) {
    free(b->data);
    stmtbuf_init(b);
}

static int stmtbuf_push(StmtBuf *b, char c) {
    if (b->len + 1 > b->cap) {
        size_t ncap = b->cap ? b->cap * 2u : 256u;
        char *n = (char *)realloc(b->data, ncap);
        if (!n) {
            return -1;
        }
        b->data = n;
        b->cap = ncap;
    }
    b->data[b->len++] = c;
    return 0;
}

static int stmtbuf_append(StmtBuf *b, const char *s, size_t n) {
    size_t i;
    for (i = 0; i < n; ++i) {
        if (stmtbuf_push(b, s[i]) != 0) {
            return -1;
        }
    }
    return 0;
}

static int stmtbuf_finish_trimmed(StmtBuf *b, char **out) {
    size_t start;
    size_t end;

    if (!b || !out) {
        return -1;
    }
    *out = NULL;
    if (b->len == 0) {
        return 0;
    }
    start = 0;
    end = b->len;
    while (start < end && isspace((unsigned char)b->data[start])) {
        start++;
    }
    while (end > start && isspace((unsigned char)b->data[end - 1])) {
        end--;
    }
    if (start >= end) {
        b->len = 0;
        return 0;
    }
    *out = (char *)malloc(end - start + 1);
    if (!*out) {
        return -1;
    }
    memcpy(*out, b->data + start, end - start);
    (*out)[end - start] = '\0';
    b->len = 0;
    return 0;
}

static void free_statement_list(char **stmts, size_t n) {
    size_t i;
    if (!stmts) {
        return;
    }
    for (i = 0; i < n; ++i) {
        free(stmts[i]);
    }
    free(stmts);
}

static int append_stmt(char ***stmts, size_t *count, size_t *cap, char *s) {
    char **ns;
    if (!s) {
        return 0;
    }
    if (*count >= *cap) {
        size_t ncap = *cap ? *cap * 2u : 8u;
        ns = (char **)realloc(*stmts, ncap * sizeof(char *));
        if (!ns) {
            free(s);
            return -1;
        }
        *stmts = ns;
        *cap = ncap;
    }
    (*stmts)[*count] = s;
    (*count)++;
    return 0;
}

/*
 * Split SQL on semicolons outside quotes, dollar-quoted blocks, and comments.
 * Line comments starting with double hyphen and C block comments are skipped for splitting.
 * Limitations: PL/pgSQL bodies that are not dollar-quoted may still split on internal
 * semicolons; use PostgreSQL dollar-quoting for functions and triggers.
 */
static int migration_sql_split_statements(const char *sql, char ***out_stmts, size_t *out_count) {
    StmtBuf cur = {0};
    char **list = NULL;
    size_t n = 0;
    size_t cap = 0;
    const char *p = sql;

    if (!sql || !out_stmts || !out_count) {
        return -1;
    }
    *out_stmts = NULL;
    *out_count = 0;

    while (*p) {
        char c = *p;

        if (c == '/' && p[1] == '*') {
            if (stmtbuf_append(&cur, p, 2) != 0) {
                goto fail;
            }
            p += 2;
            while (*p) {
                if (p[0] == '*' && p[1] == '/') {
                    if (stmtbuf_append(&cur, p, 2) != 0) {
                        goto fail;
                    }
                    p += 2;
                    break;
                }
                if (stmtbuf_push(&cur, *p) != 0) {
                    goto fail;
                }
                p++;
            }
            continue;
        }

        if (c == '-' && p[1] == '-') {
            while (*p && *p != '\n') {
                if (stmtbuf_push(&cur, *p) != 0) {
                    goto fail;
                }
                p++;
            }
            continue;
        }

        if (c == '\'') {
            if (stmtbuf_push(&cur, c) != 0) {
                goto fail;
            }
            p++;
            while (*p) {
                if (*p == '\'') {
                    if (p[1] == '\'') {
                        if (stmtbuf_append(&cur, p, 2) != 0) {
                            goto fail;
                        }
                        p += 2;
                        continue;
                    }
                    if (stmtbuf_push(&cur, *p) != 0) {
                        goto fail;
                    }
                    p++;
                    break;
                }
                if (stmtbuf_push(&cur, *p) != 0) {
                    goto fail;
                }
                p++;
            }
            continue;
        }

        if (c == '"') {
            if (stmtbuf_push(&cur, c) != 0) {
                goto fail;
            }
            p++;
            while (*p) {
                if (*p == '"') {
                    if (p[1] == '"') {
                        if (stmtbuf_append(&cur, p, 2) != 0) {
                            goto fail;
                        }
                        p += 2;
                        continue;
                    }
                    if (stmtbuf_push(&cur, *p) != 0) {
                        goto fail;
                    }
                    p++;
                    break;
                }
                if (stmtbuf_push(&cur, *p) != 0) {
                    goto fail;
                }
                p++;
            }
            continue;
        }

        if (c == '$') {
            const char *tag_open = p;
            const char *d = p + 1;
            size_t open_len;

            while (*d && *d != '$') {
                if (!(isalnum((unsigned char)*d) || *d == '_')) {
                    break;
                }
                d++;
            }
            if (*d != '$') {
                if (stmtbuf_push(&cur, c) != 0) {
                    goto fail;
                }
                p++;
                continue;
            }
            d++;
            open_len = (size_t)(d - tag_open);
            if (open_len >= 256) {
                LOG_ERROR("db.migration.runner", "dollar-quote delimiter too long");
                goto fail;
            }
            if (stmtbuf_append(&cur, tag_open, open_len) != 0) {
                goto fail;
            }
            p = d;
            while (*p) {
                if (strncmp(p, tag_open, open_len) == 0) {
                    if (stmtbuf_append(&cur, p, open_len) != 0) {
                        goto fail;
                    }
                    p += open_len;
                    goto dollar_done;
                }
                if (stmtbuf_push(&cur, *p) != 0) {
                    goto fail;
                }
                p++;
            }
            LOG_ERROR("db.migration.runner", "unclosed dollar-quoted literal");
            goto fail;
        dollar_done:
            continue;
        }

        if (c == ';') {
            char *one = NULL;
            p++;
            if (stmtbuf_finish_trimmed(&cur, &one) != 0) {
                goto fail;
            }
            if (one && append_stmt(&list, &n, &cap, one) != 0) {
                goto fail;
            }
            continue;
        }

        if (stmtbuf_push(&cur, c) != 0) {
            goto fail;
        }
        p++;
    }

    {
        char *one = NULL;
        if (stmtbuf_finish_trimmed(&cur, &one) != 0) {
            goto fail;
        }
        if (one && append_stmt(&list, &n, &cap, one) != 0) {
            goto fail;
        }
    }

    stmtbuf_free(&cur);
    *out_stmts = list;
    *out_count = n;
    return 0;

fail:
    stmtbuf_free(&cur);
    free_statement_list(list, n);
    return -1;
}

static int cmp_migration_file(const void *a, const void *b) {
    const MigrationFile *fa = (const MigrationFile *)a;
    const MigrationFile *fb = (const MigrationFile *)b;
    return strcmp(fa->version, fb->version);
}

static int is_digit14(const char *s) {
    int i;
    if (!s) {
        return 0;
    }
    for (i = 0; i < 14; ++i) {
        if (!isdigit((unsigned char)s[i])) {
            return 0;
        }
    }
    return s[14] == '_';
}

static int is_snake_tail(const char *s, size_t len) {
    size_t i;
    for (i = 0; i < len; ++i) {
        char c = s[i];
        if (!(islower((unsigned char)c) || isdigit((unsigned char)c) || c == '_')) {
            return 0;
        }
    }
    return len > 0;
}

static int parse_migration_filename(const char *filename, MigrationFile *out) {
    const char *underscore;
    size_t base_len;
    size_t tail_len;

    if (!filename || !out) {
        return -1;
    }
    memset(out, 0, sizeof(*out));

    base_len = strlen(filename);
    if (base_len < 5 || strcmp(filename + (base_len - 4), ".sql") != 0) {
        return -1;
    }
    base_len -= 4;
    if (base_len >= sizeof(out->filepath)) {
        return -1;
    }
    if (!is_digit14(filename)) {
        return -1;
    }
    underscore = filename + 14;
    if (*underscore != '_') {
        return -1;
    }
    tail_len = base_len - 15;
    if (!is_snake_tail(underscore + 1, tail_len)) {
        return -1;
    }

    memcpy(out->version, filename, 14);
    out->version[14] = '\0';
    if (tail_len >= sizeof(out->name)) {
        return -1;
    }
    memcpy(out->name, underscore + 1, tail_len);
    out->name[tail_len] = '\0';
    return 0;
}

int db_migration_scan_directory(const char *dir, MigrationFile **out_files, size_t *out_n) {
    DIR *d;
    struct dirent *ent;
    MigrationFile *arr = NULL;
    size_t n = 0;
    size_t cap = 0;

    if (!dir || !out_files || !out_n) {
        return -1;
    }
    *out_files = NULL;
    *out_n = 0;

    d = opendir(dir);
    if (!d) {
        if (errno == ENOENT) {
            LOG_WARN("db.migration.runner", "migration directory does not exist: %s", dir);
            return 0;
        }
        LOG_ERROR("db.migration.runner", "cannot open migration directory '%s': %s", dir, strerror(errno));
        return -1;
    }

    while ((ent = readdir(d)) != NULL) {
        MigrationFile mf;
        MigrationFile *grow;
        char path[768];

        if (ent->d_name[0] == '.') {
            continue;
        }
        if (parse_migration_filename(ent->d_name, &mf) != 0) {
            continue;
        }
        if (snprintf(path, sizeof(path), "%s/%s", dir, ent->d_name) >= (int)sizeof(path)) {
            closedir(d);
            free(arr);
            return -1;
        }
        memcpy(mf.filepath, path, strlen(path) + 1);

        if (n >= cap) {
            size_t ncap = cap ? cap * 2u : 8u;
            grow = (MigrationFile *)realloc(arr, ncap * sizeof(MigrationFile));
            if (!grow) {
                closedir(d);
                free(arr);
                return -1;
            }
            arr = grow;
            cap = ncap;
        }
        arr[n++] = mf;
    }
    closedir(d);

    if (n > 1) {
        qsort(arr, n, sizeof(MigrationFile), cmp_migration_file);
    }
    *out_files = arr;
    *out_n = n;
    return 0;
}

static long elapsed_ms(const struct timespec *a, const struct timespec *b) {
    long ms;
    ms = (long)(b->tv_sec - a->tv_sec) * 1000L;
    ms += (long)(b->tv_nsec - a->tv_nsec) / 1000000L;
    if (ms < 0) {
        ms = 0;
    }
    return ms;
}

const char *db_migration_default_directory(void) {
    const char *e = getenv("CORTEX_MIGRATE_DIR");
    if (e && e[0] != '\0') {
        return e;
    }
    return "db/migrate";
}

MigrationResult db_migration_migrate_sql_files(DbConnection *conn) {
    MigrationResult res = {0};
    MigrationFile *files = NULL;
    size_t i;
    size_t n = 0;

    if (!conn) {
        res.errors++;
        return res;
    }

    if (db_migration_ensure_table(conn) != 0) {
        res.errors++;
        return res;
    }

    if (db_migration_scan_directory(db_migration_default_directory(), &files, &n) != 0) {
        res.errors++;
        return res;
    }
    res.total_files = (int)n;

    for (i = 0; i < n; ++i) {
        MigrationSQL parsed = {0};
        int applied = 0;
        struct timespec t0;
        struct timespec t1;
        long ms;

        if (db_migration_is_applied(conn, files[i].version, &applied) != 0) {
            res.errors++;
            free(files);
            return res;
        }
        if (applied) {
            continue;
        }

        if (migration_sql_parse(files[i].filepath, &parsed) != 0) {
            res.errors++;
            free(files);
            return res;
        }

        clock_gettime(CLOCK_MONOTONIC, &t0);
        LOG_INFO("db.migration", "== %s_%s: migrating ==", files[i].version, files[i].name);

        if (db_connection_exec(conn, "BEGIN;") != 0) {
            migration_sql_free(&parsed);
            res.errors++;
            free(files);
            return res;
        }

        {
            char **stmts = NULL;
            size_t k;
            size_t sn = 0;
            if (migration_sql_split_statements(parsed.up_sql, &stmts, &sn) != 0) {
                (void)db_connection_exec(conn, "ROLLBACK;");
                migration_sql_free(&parsed);
                res.errors++;
                free(files);
                return res;
            }
            for (k = 0; k < sn; ++k) {
                if (!stmts[k] || stmts[k][0] == '\0') {
                    continue;
                }
                if (db_connection_exec(conn, stmts[k]) != 0) {
                    LOG_ERROR("db.migration.runner", "migration %s failed on: %s", files[i].filepath, stmts[k]);
                    (void)db_connection_exec(conn, "ROLLBACK;");
                    free_statement_list(stmts, sn);
                    migration_sql_free(&parsed);
                    res.errors++;
                    free(files);
                    return res;
                }
            }
            free_statement_list(stmts, sn);
        }

        if (db_migration_mark_applied(conn, files[i].version, files[i].name) != 0) {
            (void)db_connection_exec(conn, "ROLLBACK;");
            migration_sql_free(&parsed);
            res.errors++;
            free(files);
            return res;
        }

        if (db_connection_exec(conn, "COMMIT;") != 0) {
            (void)db_connection_exec(conn, "ROLLBACK;");
            migration_sql_free(&parsed);
            res.errors++;
            free(files);
            return res;
        }

        clock_gettime(CLOCK_MONOTONIC, &t1);
        ms = elapsed_ms(&t0, &t1);
        LOG_INFO("db.migration", "== %s_%s: migrated (%.3fs) ==", files[i].version, files[i].name, ms / 1000.0);
        migration_sql_free(&parsed);
        res.applied++;
    }

    free(files);
    return res;
}

static MigrationFile *find_file_for_version(MigrationFile *files, size_t n, const char *version) {
    size_t i;
    for (i = 0; i < n; ++i) {
        if (strcmp(files[i].version, version) == 0) {
            return &files[i];
        }
    }
    return NULL;
}

typedef struct {
    char version[16];
} AppliedVersionRow;

MigrationResult db_migration_rollback_sql_files(DbConnection *conn, int steps) {
    MigrationResult res = {0};
    MigrationFile *disk_files = NULL;
    size_t disk_n = 0;
    DbStatement *st = NULL;
    int step;
    int remaining;
    AppliedVersionRow *rows = NULL;
    size_t nrows = 0;
    size_t rows_cap = 0;
    size_t ri;

    if (!conn || steps <= 0) {
        res.errors++;
        return res;
    }

    if (db_migration_ensure_table(conn) != 0) {
        res.errors++;
        return res;
    }

    if (db_migration_scan_directory(db_migration_default_directory(), &disk_files, &disk_n) != 0) {
        res.errors++;
        return res;
    }

    if (db_connection_prepare(
            conn,
            "SELECT version, name FROM schema_migrations WHERE version >= '19900101000000' "
            "ORDER BY version DESC",
            &st) != 0) {
        res.errors++;
        free(disk_files);
        return res;
    }

    while (1) {
        const char *ver;
        AppliedVersionRow *grow;

        step = db_statement_step(st);
        if (step == 0) {
            break;
        }
        if (step < 0) {
            res.errors++;
            db_statement_finalize(st);
            free(rows);
            free(disk_files);
            return res;
        }

        ver = db_statement_column_text(st, 0);
        if (!ver || strlen(ver) > 14) {
            res.errors++;
            db_statement_finalize(st);
            free(rows);
            free(disk_files);
            return res;
        }

        if (nrows >= rows_cap) {
            size_t ncap = rows_cap ? rows_cap * 2u : 8u;
            grow = (AppliedVersionRow *)realloc(rows, ncap * sizeof(AppliedVersionRow));
            if (!grow) {
                res.errors++;
                db_statement_finalize(st);
                free(rows);
                free(disk_files);
                return res;
            }
            rows = grow;
            rows_cap = ncap;
        }
        memcpy(rows[nrows].version, ver, strlen(ver) + 1);
        nrows++;
    }
    db_statement_finalize(st);
    st = NULL;

    remaining = steps;
    for (ri = 0; ri < nrows && remaining > 0; ++ri) {
        const char *ver = rows[ri].version;
        MigrationFile *mf;
        MigrationSQL parsed = {0};

        mf = find_file_for_version(disk_files, disk_n, ver);
        if (!mf) {
            LOG_WARN("db.migration.runner", "rollback: no migration file on disk for version %s", ver);
            remaining--;
            continue;
        }

        if (migration_sql_parse(mf->filepath, &parsed) != 0) {
            res.errors++;
            migration_sql_free(&parsed);
            free(rows);
            free(disk_files);
            return res;
        }

        if (!parsed.down_sql || parsed.down_sql[0] == '\0') {
            LOG_WARN("db.migration.runner", "rollback: skipping %s (no -- migrate:down section)", mf->filepath);
            migration_sql_free(&parsed);
            remaining--;
            continue;
        }

        if (db_connection_exec(conn, "BEGIN;") != 0) {
            migration_sql_free(&parsed);
            res.errors++;
            free(rows);
            free(disk_files);
            return res;
        }

        {
            char **stmts = NULL;
            size_t k;
            size_t sn = 0;
            if (migration_sql_split_statements(parsed.down_sql, &stmts, &sn) != 0) {
                (void)db_connection_exec(conn, "ROLLBACK;");
                migration_sql_free(&parsed);
                res.errors++;
                free(rows);
                free(disk_files);
                return res;
            }
            for (k = 0; k < sn; ++k) {
                if (!stmts[k] || stmts[k][0] == '\0') {
                    continue;
                }
                if (db_connection_exec(conn, stmts[k]) != 0) {
                    LOG_ERROR("db.migration.runner", "rollback statement failed: %s", stmts[k]);
                    (void)db_connection_exec(conn, "ROLLBACK;");
                    free_statement_list(stmts, sn);
                    migration_sql_free(&parsed);
                    res.errors++;
                    free(rows);
                    free(disk_files);
                    return res;
                }
            }
            free_statement_list(stmts, sn);
        }

        if (db_migration_mark_rolled_back(conn, ver) != 0) {
            (void)db_connection_exec(conn, "ROLLBACK;");
            migration_sql_free(&parsed);
            res.errors++;
            free(rows);
            free(disk_files);
            return res;
        }

        if (db_connection_exec(conn, "COMMIT;") != 0) {
            (void)db_connection_exec(conn, "ROLLBACK;");
            migration_sql_free(&parsed);
            res.errors++;
            free(rows);
            free(disk_files);
            return res;
        }

        LOG_INFO("db.migration", "== %s: rolled back ==", ver);
        migration_sql_free(&parsed);
        res.rolled_back++;
        remaining--;
    }

    free(rows);
    free(disk_files);
    return res;
}

MigrationResult db_migration_status_sql_files(DbConnection *conn, FILE *out) {
    MigrationResult res = {0};
    MigrationFile *files = NULL;
    size_t n = 0;
    size_t i;
    const char *dir = db_migration_default_directory();

    if (!conn || !out) {
        res.errors++;
        return res;
    }

    fprintf(out, "database: %s\n\n", dir);
    fprintf(out, " %-6s  %-14s  %s\n", "Status", "Version", "Name");
    fprintf(out, "--------------------------------------------------\n");

    if (db_migration_ensure_table(conn) != 0) {
        res.errors++;
        return res;
    }

    if (db_migration_scan_directory(dir, &files, &n) != 0) {
        res.errors++;
        return res;
    }

    for (i = 0; i < n; ++i) {
        int applied = 0;
        if (db_migration_is_applied(conn, files[i].version, &applied) != 0) {
            res.errors++;
            free(files);
            return res;
        }
        fprintf(out, " %-6s  %-14s  %s\n", applied ? "up" : "down", files[i].version, files[i].name);
        if (applied) {
            res.applied++;
        } else {
            res.pending++;
        }
    }

    free(files);
    return res;
}

static int validate_snake_name(const char *name) {
    const char *p;
    if (!name || name[0] == '\0') {
        return -1;
    }
    if (!islower((unsigned char)name[0])) {
        return -1;
    }
    for (p = name; *p; ++p) {
        if (!(islower((unsigned char)*p) || isdigit((unsigned char)*p) || *p == '_')) {
            return -1;
        }
    }
    return 0;
}

int db_migration_generate(const char *name) {
    struct timespec ts;
    struct tm tm_buf;
    time_t t;
    char ver[32];
    char path[640];
    char rel[640];
    FILE *f;
    const char *dir = db_migration_default_directory();

    if (validate_snake_name(name) != 0) {
        LOG_ERROR("db.migration.generate", "invalid migration name (use snake_case: ^[a-z][a-z0-9_]*$)");
        return -1;
    }

    if (mkdir("db", 0755) == -1 && errno != EEXIST) {
        LOG_ERROR("db.migration.generate", "cannot create db/: %s", strerror(errno));
        return -1;
    }
    if (mkdir(dir, 0755) == -1 && errno != EEXIST) {
        LOG_ERROR("db.migration.generate", "cannot create %s: %s", dir, strerror(errno));
        return -1;
    }

    clock_gettime(CLOCK_REALTIME, &ts);
    t = ts.tv_sec;
    if (localtime_r(&t, &tm_buf) == NULL) {
        return -1;
    }
    if (strftime(ver, sizeof(ver), "%Y%m%d%H%M%S", &tm_buf) == 0) {
        return -1;
    }

    if (snprintf(path, sizeof(path), "%s/%s_%s.sql", dir, ver, name) >= (int)sizeof(path)) {
        return -1;
    }
    if (snprintf(rel, sizeof(rel), "%s/%s_%s.sql", dir, ver, name) >= (int)sizeof(rel)) {
        return -1;
    }

    if (access(path, F_OK) == 0) {
        LOG_ERROR("db.migration.generate", "refusing to overwrite existing file: %s", path);
        return -1;
    }

    f = fopen(path, "w");
    if (!f) {
        LOG_ERROR("db.migration.generate", "cannot create %s: %s", path, strerror(errno));
        return -1;
    }

    fprintf(f, "-- Migration: %s_%s\n\n", ver, name);
    fprintf(f, "-- migrate:up\n\n\n");
    fprintf(f, "-- migrate:down\n\n");
    if (fclose(f) != 0) {
        return -1;
    }

    printf("      create %s\n", rel);
    fflush(stdout);
    return 0;
}
