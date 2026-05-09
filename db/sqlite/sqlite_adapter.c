#define _POSIX_C_SOURCE 200809L

#include "sqlite_adapter.h"
#include "../db_connection_priv.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <sqlite3.h>

#include "cortex_error.h"
#include "pulse.h"

#define SQLITE_SLOW_QUERY_MS 100

static long sqlite_elapsed_ms(const struct timespec *start,
                              const struct timespec *end)
{
    long ms;

    ms  = (long)(end->tv_sec  - start->tv_sec)  * 1000L;
    ms += (long)(end->tv_nsec - start->tv_nsec) / 1000000L;
    if (ms < 0) {
        ms = 0;
    }
    return ms;
}

static void sqlite_log_query_outcome(const char *sql,
                                     long elapsed_ms,
                                     int rc,
                                     const char *errmsg)
{
    char rc_buf[16];
    char dur_buf[24];

    snprintf(rc_buf,  sizeof(rc_buf),  "%d",   rc);
    snprintf(dur_buf, sizeof(dur_buf), "%ldms", elapsed_ms);

    if (rc != SQLITE_OK) {
        pulse_log_fields(PULSE_ERROR,
                         "db.sqlite",
                         "query failed",
                         "sql",      sql ? sql : "",
                         "duration", dur_buf,
                         "rc",       rc_buf,
                         "error",    errmsg ? errmsg : "(unknown)",
                         NULL);
        return;
    }

    if (elapsed_ms > SQLITE_SLOW_QUERY_MS) {
        pulse_log_fields(PULSE_WARN,
                         "db.sqlite",
                         "slow query",
                         "sql",      sql ? sql : "",
                         "duration", dur_buf,
                         "rc",       rc_buf,
                         NULL);
    } else {
        pulse_log_fields(PULSE_DEBUG,
                         "db.sqlite",
                         "query ok",
                         "sql",      sql ? sql : "",
                         "duration", dur_buf,
                         "rc",       rc_buf,
                         NULL);
    }
}

static CortexErrCode sqlite_exec_rc_classify(int sqlite_rc)
{
    int primary = sqlite_rc & 0xff;

    if (primary == SQLITE_BUSY || primary == SQLITE_LOCKED) {
        return CORTEX_ERR_DB_BUSY;
    }
    if (primary == SQLITE_CONSTRAINT) {
        return CORTEX_ERR_DB_CONSTRAINT;
    }
    return CORTEX_ERR_DB_EXEC;
}

static CortexErrCode sqlite_prepare_rc_classify(int sqlite_rc)
{
    int primary = sqlite_rc & 0xff;

    if (primary == SQLITE_BUSY || primary == SQLITE_LOCKED) {
        return CORTEX_ERR_DB_BUSY;
    }
    return CORTEX_ERR_DB_PREPARE;
}

static void sqlite_conn_close(void *ctx)
{
    sqlite3 *db = (sqlite3 *)ctx;

    if (db) {
        sqlite3_close(db);
    }
}

static int sqlite_op_exec(void *ctx, const char *sql)
{
    sqlite3 *db = (sqlite3 *)ctx;
    char *errmsg = NULL;
    int rc;
    struct timespec t_start;
    struct timespec t_end;
    long elapsed_ms;

    if (!db || !sql) {
        CORTEX_SET_ERROR(CORTEX_ERR_INVALID_ARGUMENT, "sqlite:db_connection_exec",
                         "missing connection handle or sql");
        return -1;
    }

    clock_gettime(CLOCK_MONOTONIC, &t_start);
    rc = sqlite3_exec(db, sql, NULL, NULL, &errmsg);
    clock_gettime(CLOCK_MONOTONIC, &t_end);
    elapsed_ms = sqlite_elapsed_ms(&t_start, &t_end);

    if (rc != SQLITE_OK) {
        CortexErrCode classification = sqlite_exec_rc_classify(rc);
        const char *em = errmsg ? errmsg : sqlite3_errmsg(db);
        sqlite_log_query_outcome(sql, elapsed_ms, rc, em);
        CORTEX_SET_ERRORF(classification, "sqlite:db_connection_exec", "%s (SQLite rc=%d)",
                          em ? em : "(no message)", rc);
        if (errmsg) {
            sqlite3_free(errmsg);
        }
        return -1;
    }
    sqlite_log_query_outcome(sql, elapsed_ms, SQLITE_OK, NULL);
    cortex_clear_error();
    return 0;
}

static int sqlite_op_prepare(void *ctx, const char *sql, DbStatement *stmt)
{
    sqlite3 *db = (sqlite3 *)ctx;
    sqlite3_stmt *st = NULL;
    int rc;

    if (!db || !sql || !stmt) {
        CORTEX_SET_ERROR(CORTEX_ERR_INVALID_ARGUMENT, "sqlite:db_connection_prepare",
                         "missing connection handle, sql, or statement");
        return -1;
    }

    rc = sqlite3_prepare_v2(db, sql, -1, &st, NULL);
    if (rc != SQLITE_OK) {
        CortexErrCode classification = sqlite_prepare_rc_classify(rc);
        const char *em = sqlite3_errmsg(db);
        sqlite_log_query_outcome(sql, 0, rc, em);
        CORTEX_SET_ERRORF(classification, "sqlite:db_connection_prepare",
                          "Prepare failed: %s (SQLite rc=%d)", em, rc);
        return -1;
    }

    stmt->stmt_ctx = st;
    cortex_clear_error();
    return 0;
}

static int sqlite_op_step(DbStatement *stmt)
{
    sqlite3_stmt *st;
    int rc;

    if (!stmt || !stmt->stmt_ctx) {
        CORTEX_SET_ERROR(CORTEX_ERR_INVALID_ARGUMENT, "sqlite:db_statement_step",
                         "invalid statement");
        return -1;
    }

    st = (sqlite3_stmt *)stmt->stmt_ctx;
    rc = sqlite3_step(st);
    if (rc == SQLITE_ROW) {
        cortex_clear_error();
        return 1;
    }
    if (rc == SQLITE_DONE) {
        cortex_clear_error();
        return 0;
    }
    {
        CortexErrCode classification = sqlite_exec_rc_classify(rc);
        const char *em = sqlite3_errmsg(sqlite3_db_handle(st));
        char rc_buf[16];
        snprintf(rc_buf, sizeof(rc_buf), "%d", rc);
        pulse_log_fields(PULSE_ERROR,
                         "db.sqlite",
                         "step failed",
                         "rc",    rc_buf,
                         "error", em ? em : "(unknown)",
                         NULL);
        CORTEX_SET_ERRORF(classification, "sqlite:db_statement_step", "%s (SQLite rc=%d)",
                          em, rc);
        return -1;
    }
}

static void sqlite_op_finalize_stmt(DbStatement *stmt)
{
    if (!stmt) {
        return;
    }
    if (stmt->stmt_ctx) {
        sqlite3_finalize((sqlite3_stmt *)stmt->stmt_ctx);
        stmt->stmt_ctx = NULL;
    }
}

static int sqlite_op_bind_int(DbStatement *stmt, int index, int value)
{
    sqlite3_stmt *st;
    int rc;

    if (!stmt || !stmt->stmt_ctx || index <= 0) {
        CORTEX_SET_ERROR(CORTEX_ERR_INVALID_ARGUMENT, "sqlite:db_statement_bind_int",
                         "invalid statement or bind index");
        return -1;
    }

    st = (sqlite3_stmt *)stmt->stmt_ctx;
    rc = sqlite3_bind_int(st, index, value);
    if (rc != SQLITE_OK) {
        CORTEX_SET_ERRORF(CORTEX_ERR_DB_PREPARE, "sqlite:db_statement_bind_int",
                          "bind failed: %s (SQLite rc=%d)",
                          sqlite3_errmsg(sqlite3_db_handle(st)), rc);
        return -1;
    }
    cortex_clear_error();
    return 0;
}

static int sqlite_op_bind_text(DbStatement *stmt, int index, const char *value)
{
    sqlite3_stmt *st;
    int rc;

    if (!stmt || !stmt->stmt_ctx || index <= 0) {
        CORTEX_SET_ERROR(CORTEX_ERR_INVALID_ARGUMENT, "sqlite:db_statement_bind_text",
                         "invalid statement or bind index");
        return -1;
    }

    st = (sqlite3_stmt *)stmt->stmt_ctx;
    rc = sqlite3_bind_text(st, index, value ? value : "", -1, SQLITE_TRANSIENT);
    if (rc != SQLITE_OK) {
        CORTEX_SET_ERRORF(CORTEX_ERR_DB_PREPARE, "sqlite:db_statement_bind_text",
                          "bind failed: %s (SQLite rc=%d)",
                          sqlite3_errmsg(sqlite3_db_handle(st)), rc);
        return -1;
    }
    cortex_clear_error();
    return 0;
}

static int sqlite_op_column_int(DbStatement *stmt, int col_index)
{
    if (!stmt || !stmt->stmt_ctx || col_index < 0) {
        return 0;
    }
    return sqlite3_column_int((sqlite3_stmt *)stmt->stmt_ctx, col_index);
}

static const char *sqlite_op_column_text(DbStatement *stmt, int col_index)
{
    if (!stmt || !stmt->stmt_ctx || col_index < 0) {
        return NULL;
    }
    return (const char *)sqlite3_column_text((sqlite3_stmt *)stmt->stmt_ctx, col_index);
}

static const DbConnectionOps cortex_sqlite_ops = {
    .conn_close      = sqlite_conn_close,
    .exec            = sqlite_op_exec,
    .prepare         = sqlite_op_prepare,
    .step            = sqlite_op_step,
    .finalize_stmt   = sqlite_op_finalize_stmt,
    .bind_int        = sqlite_op_bind_int,
    .bind_text       = sqlite_op_bind_text,
    .column_int      = sqlite_op_column_int,
    .column_text     = sqlite_op_column_text,
};

int cortex_sqlite_connection_open(const char *path, DbConnection **out)
{
    sqlite3 *db;
    int rc;
    DbConnection *conn;

    if (!path || path[0] == '\0' || !out) {
        CORTEX_SET_ERROR(CORTEX_ERR_INVALID_ARGUMENT, "sqlite:db_connection_open",
                         "invalid path or output pointer");
        return -1;
    }

    *out = NULL;
    rc = sqlite3_open(path, &db);
    if (rc != SQLITE_OK) {
        const char *em = (db ? sqlite3_errmsg(db) : "sqlite_open failed");
        CORTEX_SET_ERRORF(CORTEX_ERR_DB_CONNECT, "sqlite:db_connection_open",
                          "Failed to open '%s': %s (code %d)", path, em, rc);
        if (db) {
            sqlite3_close(db);
        }
        return -1;
    }

    conn = (DbConnection *)calloc(1, sizeof(DbConnection));
    if (!conn) {
        sqlite3_close(db);
        CORTEX_SET_ERROR(CORTEX_ERR_CORE_OOM, "sqlite:db_connection_open",
                         "Out of memory allocating DbConnection wrapper");
        return -1;
    }

    conn->backend = CORTEX_DB_BACKEND_SQLITE;
    conn->ops     = &cortex_sqlite_ops;
    conn->ctx     = db;
    *out          = conn;
    cortex_clear_error();
    return 0;
}
