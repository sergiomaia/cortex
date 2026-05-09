#ifdef CORTEX_HAVE_POSTGRES

#include "postgres_adapter.h"
#include "../db_connection_priv.h"

#include "cortex_error.h"
#include "pulse.h"

#include <libpq-fe.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>
#include <limits.h>
#include <stdint.h>

#define PG_SLOW_QUERY_MS 100
#define PG_PARAM_MAX     32
#define PG_BIND_VALUE_BYTES 512

typedef struct PgCtx {
    PGconn  *pg;
    unsigned stmt_seq;
} PgCtx;

typedef struct PgStmt {
    PGconn   *pg;
    PGresult *res;
    int       executed;
    int       iter_row;
    int       last_row;
    int       nrows;
    int       ncols;

    int  param_count;
    char prep_name[48];
    char *converted_sql;

    uint32_t bound_mask;
    char    *param_vals[PG_PARAM_MAX];
    char     param_buf[PG_PARAM_MAX][PG_BIND_VALUE_BYTES];
} PgStmt;

static long pg_elapsed_ms(const struct timespec *start, const struct timespec *end)
{
    long ms;

    ms  = (long)(end->tv_sec  - start->tv_sec)  * 1000L;
    ms += (long)(end->tv_nsec - start->tv_nsec) / 1000000L;
    if (ms < 0) {
        ms = 0;
    }
    return ms;
}

static void pg_log_query_outcome(const char *sql, long elapsed_ms, int ok, const char *err)
{
    char dur_buf[24];
    snprintf(dur_buf, sizeof(dur_buf), "%ldms", elapsed_ms);

    if (!ok) {
        pulse_log_fields(PULSE_ERROR,
                         "db.postgres",
                         "query failed",
                         "sql",      sql ? sql : "",
                         "duration", dur_buf,
                         "error",    err ? err : "(unknown)",
                         NULL);
        return;
    }
    if (elapsed_ms > PG_SLOW_QUERY_MS) {
        pulse_log_fields(PULSE_WARN,
                         "db.postgres",
                         "slow query",
                         "sql",      sql ? sql : "",
                         "duration", dur_buf,
                         NULL);
    } else {
        pulse_log_fields(PULSE_DEBUG,
                         "db.postgres",
                         "query ok",
                         "sql",      sql ? sql : "",
                         "duration", dur_buf,
                         NULL);
    }
}

/*
 * Rewrite SQLite-style placeholders (? and ?nn) into PostgreSQL $n form.
 * `?` inside single-quoted string literals is copied verbatim.
 * Returns allocated string or NULL. *max_param receives highest index used.
 */
static char *pg_rewrite_sqlite_placeholders(const char *in, int *max_param)
{
    size_t cap;
    char *out;
    size_t o = 0;
    size_t i = 0;
    int next_anon = 1;
    int hi = 0;
    int in_str = 0;

    if (!in || !max_param) {
        return NULL;
    }
    *max_param = 0;
    cap = strlen(in) * 4u + 128u;
    if (cap < 128u) {
        cap = 128u;
    }
    out = (char *)malloc(cap);
    if (!out) {
        return NULL;
    }

    while (in[i] != '\0') {
        if (in_str) {
            if (in[i] == '\'' && in[i + 1] == '\'') {
                if (o + 2u >= cap) {
                    char *n;
                    cap = cap * 2u + 64u;
                    n = (char *)realloc(out, cap);
                    if (!n) { free(out); return NULL; }
                    out = n;
                }
                out[o++] = '\'';
                out[o++] = '\'';
                i += 2;
                continue;
            }
            if (in[i] == '\'') {
                in_str = 0;
            }
            if (o + 1u >= cap) {
                char *n;
                cap = cap * 2u + 64u;
                n = (char *)realloc(out, cap);
                if (!n) { free(out); return NULL; }
                out = n;
            }
            out[o++] = in[i];
            i++;
            continue;
        }

        if (in[i] == '\'') {
            in_str = 1;
            if (o + 1u >= cap) {
                char *n;
                cap = cap * 2u + 64u;
                n = (char *)realloc(out, cap);
                if (!n) { free(out); return NULL; }
                out = n;
            }
            out[o++] = in[i];
            i++;
            continue;
        }

        if (in[i] == '?') {
            char subst[24];
            int idx;
            int n;
            size_t j;

            i++;
            if (isdigit((unsigned char)in[i])) {
                idx = 0;
                while (isdigit((unsigned char)in[i])) {
                    if (idx > INT_MAX / 10) {
                        free(out);
                        return NULL;
                    }
                    idx = idx * 10 + (in[i] - '0');
                    i++;
                }
            } else {
                idx = next_anon;
                next_anon++;
            }
            if (idx <= 0 || idx > PG_PARAM_MAX) {
                free(out);
                return NULL;
            }
            if (idx > hi) {
                hi = idx;
            }

            n = snprintf(subst, sizeof(subst), "$%d", idx);
            if (n <= 0 || (size_t)n >= sizeof(subst)) {
                free(out);
                return NULL;
            }
            for (j = 0; j < (size_t)n; j++) {
                if (o + 2u >= cap) {
                    char *nout;
                    cap = cap * 2u + 64u;
                    nout = (char *)realloc(out, cap);
                    if (!nout) { free(out); return NULL; }
                    out = nout;
                }
                out[o++] = subst[j];
            }
            continue;
        }

        if (o + 2u >= cap) {
            char *n;
            cap = cap * 2u + 64u;
            n = (char *)realloc(out, cap);
            if (!n) { free(out); return NULL; }
            out = n;
        }
        out[o++] = in[i];
        i++;
    }

    if (o + 1u >= cap) {
        char *n;
        cap = o + 2u;
        n = (char *)realloc(out, cap);
        if (!n) {
            free(out);
            return NULL;
        }
        out = n;
    }
    out[o] = '\0';
    *max_param = hi;
    return out;
}

static void pg_stmt_reset_bindings(PgStmt *ps)
{
    int i;

    if (!ps) {
        return;
    }
    ps->bound_mask = 0;
    for (i = 0; i < PG_PARAM_MAX; i++) {
        ps->param_vals[i] = NULL;
        ps->param_buf[i][0] = '\0';
    }
}

static void cortex_pg_app_name_from_cwd(char *out, size_t cap)
{
    char cwd[PATH_MAX];
    char *slash;
    size_t w = 0;
    size_t i;

    if (!out || cap == 0) {
        return;
    }
    out[0] = '\0';
    if (!getcwd(cwd, sizeof(cwd))) {
        (void)snprintf(out, cap, "app");
        return;
    }
    slash = strrchr(cwd, '/');
    if (slash && slash[1] != '\0') {
        for (i = (size_t)(slash - cwd + 1); cwd[i] != '\0'; i++) {
            char c = cwd[i];
            if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_') {
                if (w + 1 < cap) {
                    out[w++] = (char)(c >= 'A' && c <= 'Z' ? c + ('a' - 'A') : c);
                }
            }
        }
        out[w] = '\0';
    }
    if (out[0] == '\0') {
        (void)snprintf(out, cap, "app");
        return;
    }
    if (strlen(out) > 48) {
        out[48] = '\0';
    }
}

static void pg_conn_close(void *ctx)
{
    PgCtx *c = (PgCtx *)ctx;

    if (c) {
        if (c->pg) {
            PQfinish(c->pg);
        }
        c->pg = NULL;
        free(c);
    }
}

static int pg_op_exec(void *ctx, const char *sql)
{
    PGconn *pg = ((PgCtx *)ctx)->pg;
    struct timespec t_start;
    struct timespec t_end;
    long elapsed_ms;
    PGresult *res;
    ExecStatusType st;

    if (!pg || !sql) {
        CORTEX_SET_ERROR(CORTEX_ERR_INVALID_ARGUMENT, "postgres:db_connection_exec",
                         "missing connection or sql");
        return -1;
    }

    clock_gettime(CLOCK_MONOTONIC, &t_start);
    res = PQexec(pg, sql);
    clock_gettime(CLOCK_MONOTONIC, &t_end);
    elapsed_ms = pg_elapsed_ms(&t_start, &t_end);

    st = PQresultStatus(res);
    if (st != PGRES_COMMAND_OK && st != PGRES_TUPLES_OK) {
        const char *em = PQresultErrorMessage(res);
        pg_log_query_outcome(sql, elapsed_ms, 0, em);
        CORTEX_SET_ERRORF(CORTEX_ERR_DB_EXEC, "postgres:db_connection_exec", "%s",
                          em && em[0] ? em : "PQexec failed");
        PQclear(res);
        return -1;
    }
    pg_log_query_outcome(sql, elapsed_ms, 1, NULL);
    PQclear(res);
    cortex_clear_error();
    return 0;
}

static int pg_op_prepare(void *ctx, const char *sql, DbStatement *stmt)
{
    PgCtx *c = (PgCtx *)ctx;
    PGconn *pg;
    char *converted;
    int maxp;
    PgStmt *ps;
    int rc;
    PGresult *res;

    if (!c || !c->pg || !sql || !stmt) {
        CORTEX_SET_ERROR(CORTEX_ERR_INVALID_ARGUMENT, "postgres:db_connection_prepare",
                         "invalid arguments");
        return -1;
    }
    pg = c->pg;

    converted = pg_rewrite_sqlite_placeholders(sql, &maxp);
    if (!converted) {
        CORTEX_SET_ERROR(CORTEX_ERR_DB_PREPARE, "postgres:db_connection_prepare",
                         "SQL placeholder rewrite failed (too many or invalid ? placeholders)");
        return -1;
    }

    ps = (PgStmt *)calloc(1, sizeof(PgStmt));
    if (!ps) {
        free(converted);
        CORTEX_SET_ERROR(CORTEX_ERR_CORE_OOM, "postgres:db_connection_prepare",
                         "Out of memory allocating PgStmt");
        return -1;
    }

    ps->pg = pg;
    ps->converted_sql = converted;
    ps->param_count = maxp;
    pg_stmt_reset_bindings(ps);

    (void)snprintf(ps->prep_name, sizeof(ps->prep_name), "cx_%u", c->stmt_seq);
    c->stmt_seq++;

    res = PQprepare(pg, ps->prep_name, converted, 0, NULL);
    rc = PQresultStatus(res);
    if (rc != PGRES_COMMAND_OK) {
        const char *em = PQresultErrorMessage(res);
        CORTEX_SET_ERRORF(CORTEX_ERR_DB_PREPARE, "postgres:db_connection_prepare",
                          "PQprepare failed: %s", em && em[0] ? em : "unknown");
        PQclear(res);
        free(converted);
        free(ps);
        return -1;
    }
    PQclear(res);

    stmt->stmt_ctx = ps;
    cortex_clear_error();
    return 0;
}

static int pg_op_step(DbStatement *stmt)
{
    PgStmt *ps;
    ExecStatusType st;

    if (!stmt || !stmt->stmt_ctx) {
        CORTEX_SET_ERROR(CORTEX_ERR_INVALID_ARGUMENT, "postgres:db_statement_step",
                         "invalid statement");
        return -1;
    }
    ps = (PgStmt *)stmt->stmt_ctx;

    if (!ps->executed) {
        int i;
        const char *paramv[PG_PARAM_MAX];

        for (i = 0; i < ps->param_count; i++) {
            if (ps->bound_mask & (1u << i)) {
                paramv[i] = ps->param_vals[i];
            } else {
                paramv[i] = NULL;
            }
        }

        if (ps->res) {
            PQclear(ps->res);
            ps->res = NULL;
        }

        ps->res = PQexecPrepared(ps->pg, ps->prep_name, ps->param_count,
                                 ps->param_count > 0 ? paramv : NULL,
                                 NULL, NULL, 0);
        st = PQresultStatus(ps->res);
        if (st != PGRES_COMMAND_OK && st != PGRES_TUPLES_OK) {
            const char *em = PQresultErrorMessage(ps->res);
            CORTEX_SET_ERRORF(CORTEX_ERR_DB_EXEC, "postgres:db_statement_step",
                              "%s", em && em[0] ? em : "PQexecPrepared failed");
            return -1;
        }

        ps->executed = 1;
        ps->iter_row = 0;
        ps->last_row = 0;
        ps->nrows = PQntuples(ps->res);
        ps->ncols = PQnfields(ps->res);

        if (st == PGRES_COMMAND_OK) {
            cortex_clear_error();
            return 0;
        }
    }

    st = PQresultStatus(ps->res);
    if (st == PGRES_TUPLES_OK && ps->iter_row < ps->nrows) {
        ps->last_row = ps->iter_row;
        ps->iter_row++;
        cortex_clear_error();
        return 1;
    }

    cortex_clear_error();
    return 0;
}

static void pg_op_finalize_stmt(DbStatement *stmt)
{
    PgStmt *ps;
    char dq[96];
    PGresult *r;

    if (!stmt || !stmt->stmt_ctx) {
        return;
    }
    ps = (PgStmt *)stmt->stmt_ctx;

    if (ps->res) {
        PQclear(ps->res);
        ps->res = NULL;
    }

    if (ps->prep_name[0] != '\0' && ps->pg) {
        (void)snprintf(dq, sizeof(dq), "DEALLOCATE %s", ps->prep_name);
        r = PQexec(ps->pg, dq);
        if (r) {
            PQclear(r);
        }
    }

    if (ps->converted_sql) {
        free(ps->converted_sql);
        ps->converted_sql = NULL;
    }
    free(ps);
    stmt->stmt_ctx = NULL;
}

static int pg_op_bind_int(DbStatement *stmt, int index, int value)
{
    PgStmt *ps;
    int n;

    if (!stmt || !stmt->stmt_ctx || index <= 0 || index > PG_PARAM_MAX) {
        CORTEX_SET_ERROR(CORTEX_ERR_INVALID_ARGUMENT, "postgres:db_statement_bind_int",
                         "invalid statement or bind index");
        return -1;
    }
    ps = (PgStmt *)stmt->stmt_ctx;
    if (index > ps->param_count) {
        CORTEX_SET_ERROR(CORTEX_ERR_INVALID_ARGUMENT, "postgres:db_statement_bind_int",
                         "bind index exceeds placeholder count");
        return -1;
    }

    n = snprintf(ps->param_buf[index - 1], sizeof(ps->param_buf[index - 1]), "%d", value);
    if (n < 0 || (size_t)n >= sizeof(ps->param_buf[index - 1])) {
        CORTEX_SET_ERROR(CORTEX_ERR_DB_PREPARE, "postgres:db_statement_bind_int",
                         "integer bind value too long");
        return -1;
    }
    ps->param_vals[index - 1] = ps->param_buf[index - 1];
    ps->bound_mask |= (1u << (index - 1));
    cortex_clear_error();
    return 0;
}

static int pg_op_bind_text(DbStatement *stmt, int index, const char *value)
{
    PgStmt *ps;
    const char *src = value ? value : "";

    if (!stmt || !stmt->stmt_ctx || index <= 0 || index > PG_PARAM_MAX) {
        CORTEX_SET_ERROR(CORTEX_ERR_INVALID_ARGUMENT, "postgres:db_statement_bind_text",
                         "invalid statement or bind index");
        return -1;
    }
    ps = (PgStmt *)stmt->stmt_ctx;
    if (index > ps->param_count) {
        CORTEX_SET_ERROR(CORTEX_ERR_INVALID_ARGUMENT, "postgres:db_statement_bind_text",
                         "bind index exceeds placeholder count");
        return -1;
    }

    {
        size_t maxcopy = sizeof(ps->param_buf[index - 1]) - 1u;
        size_t sl = strlen(src);
        if (sl > maxcopy) {
            CORTEX_SET_ERROR(CORTEX_ERR_DB_PREPARE, "postgres:db_statement_bind_text",
                             "text bind exceeds buffer");
            return -1;
        }
        memcpy(ps->param_buf[index - 1], src, sl + 1u);
    }
    ps->param_vals[index - 1] = ps->param_buf[index - 1];
    ps->bound_mask |= (1u << (index - 1));
    cortex_clear_error();
    return 0;
}

static int pg_op_column_int(DbStatement *stmt, int col_index)
{
    PgStmt *ps;
    const char *t;

    if (!stmt || !stmt->stmt_ctx || col_index < 0) {
        return 0;
    }
    ps = (PgStmt *)stmt->stmt_ctx;
    if (!ps->res || col_index >= ps->ncols) {
        return 0;
    }
    if (PQgetisnull(ps->res, ps->last_row, col_index)) {
        return 0;
    }
    t = PQgetvalue(ps->res, ps->last_row, col_index);
    if (!t) {
        return 0;
    }
    return atoi(t);
}

static const char *pg_op_column_text(DbStatement *stmt, int col_index)
{
    PgStmt *ps;

    if (!stmt || !stmt->stmt_ctx || col_index < 0) {
        return NULL;
    }
    ps = (PgStmt *)stmt->stmt_ctx;
    if (!ps->res || col_index >= ps->ncols) {
        return NULL;
    }
    if (PQgetisnull(ps->res, ps->last_row, col_index)) {
        return NULL;
    }
    return PQgetvalue(ps->res, ps->last_row, col_index);
}

static const DbConnectionOps cortex_postgres_ops = {
    .conn_close    = pg_conn_close,
    .exec          = pg_op_exec,
    .prepare       = pg_op_prepare,
    .step          = pg_op_step,
    .finalize_stmt = pg_op_finalize_stmt,
    .bind_int      = pg_op_bind_int,
    .bind_text     = pg_op_bind_text,
    .column_int    = pg_op_column_int,
    .column_text   = pg_op_column_text,
};

static DbConnection *postgres_wrap_connection(PGconn *pg)
{
    DbConnection *conn;
    PgCtx *ctx;

    if (!pg) {
        return NULL;
    }

    ctx = (PgCtx *)calloc(1, sizeof(PgCtx));
    if (!ctx) {
        PQfinish(pg);
        CORTEX_SET_ERROR(CORTEX_ERR_CORE_OOM, "postgres:connect",
                         "Out of memory allocating PgCtx");
        return NULL;
    }
    ctx->pg = pg;

    conn = (DbConnection *)calloc(1, sizeof(DbConnection));
    if (!conn) {
        PQfinish(pg);
        free(ctx);
        CORTEX_SET_ERROR(CORTEX_ERR_CORE_OOM, "postgres:connect",
                         "Out of memory allocating DbConnection");
        return NULL;
    }
    conn->backend = CORTEX_DB_BACKEND_POSTGRESQL;
    conn->ops     = &cortex_postgres_ops;
    conn->ctx     = ctx;
    return conn;
}

DbConnection *postgres_connect(const char *connstring)
{
    PGconn *pg;
    DbConnection *conn;

    if (!connstring || connstring[0] == '\0') {
        CORTEX_SET_ERROR(CORTEX_ERR_INVALID_ARGUMENT, "postgres:postgres_connect",
                         "connection string is empty");
        return NULL;
    }

    pg = PQconnectdb(connstring);
    if (!pg || PQstatus(pg) != CONNECTION_OK) {
        const char *em = pg ? PQerrorMessage(pg) : "alloc failed";
        CORTEX_SET_ERRORF(CORTEX_ERR_DB_CONNECT, "postgres:postgres_connect",
                          "%s", em && em[0] ? em : "PQconnectdb failed");
        if (pg) {
            PQfinish(pg);
        }
        return NULL;
    }

    conn = postgres_wrap_connection(pg);
    if (conn) {
        LOG_INFO("db",
                 "PostgreSQL connected: %s@%s:%s/%s",
                 PQuser(pg) ? PQuser(pg) : "",
                 PQhost(pg)  ? PQhost(pg)  : "",
                 PQport(pg)  ? PQport(pg)  : "",
                 PQdb(pg)    ? PQdb(pg)    : "");
        cortex_clear_error();
    }
    return conn;
}

DbConnection *postgres_connect_from_env(void)
{
    char buf[512];
    const char *existing_db;
    const char *env_name;

    existing_db = getenv("PGDATABASE");
    if (existing_db && existing_db[0] != '\0') {
        return postgres_connect("");
    }

    env_name = getenv("CORE_ENV");
    if (!env_name || env_name[0] == '\0') {
        env_name = "development";
    }

    {
        char app[64];
        cortex_pg_app_name_from_cwd(app, sizeof(app));
        if (snprintf(buf, sizeof(buf), "dbname=%s_%s", app, env_name) >= (int)sizeof(buf)) {
            CORTEX_SET_ERROR(CORTEX_ERR_DB_CONNECT, "postgres:postgres_connect_from_env",
                             "constructed db name too long");
            return NULL;
        }
    }

    return postgres_connect(buf);
}

void postgres_adapter_register(void)
{
    /* Adapter selection uses env at bootstrap/pool init; nothing to register globally. */
}

#endif /* CORTEX_HAVE_POSTGRES */
