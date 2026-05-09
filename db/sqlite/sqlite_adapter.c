#define _POSIX_C_SOURCE 200809L

#include "../db_connection.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <sqlite3.h>

#include "cortex_error.h"
#include "pulse.h"

/*
 * Slow-query threshold in milliseconds. Queries that complete faster than
 * this are logged at DEBUG level; anything slower is logged at WARN level so
 * operators can spot regressions without digging through trace output.
 */
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

struct DbConnection {
    sqlite3 *handle;
};

struct DbStatement {
    sqlite3_stmt *stmt;
};

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

int db_connection_open(const char *path, DbConnection **out)
{
    sqlite3 *db;
    int rc;

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

    *out = (DbConnection *)calloc(1, sizeof(DbConnection));
    if (!*out) {
        sqlite3_close(db);
        CORTEX_SET_ERROR(CORTEX_ERR_CORE_OOM, "sqlite:db_connection_open",
                         "Out of memory allocating DbConnection wrapper");
        return -1;
    }

    (*out)->handle = db;
    cortex_clear_error();
    return 0;
}

void db_connection_close(DbConnection *conn)
{
    if (!conn) {
        return;
    }
    if (conn->handle) {
        sqlite3_close(conn->handle);
        conn->handle = NULL;
    }
    free(conn);
}

int db_connection_exec(DbConnection *conn, const char *sql)
{
    char *errmsg = NULL;
    int rc;
    struct timespec t_start;
    struct timespec t_end;
    long elapsed_ms;

    if (!conn || !conn->handle || !sql) {
        CORTEX_SET_ERROR(CORTEX_ERR_INVALID_ARGUMENT, "sqlite:db_connection_exec",
                         "missing connection handle or sql");
        return -1;
    }

    clock_gettime(CLOCK_MONOTONIC, &t_start);
    rc = sqlite3_exec(conn->handle, sql, NULL, NULL, &errmsg);
    clock_gettime(CLOCK_MONOTONIC, &t_end);
    elapsed_ms = sqlite_elapsed_ms(&t_start, &t_end);

    if (rc != SQLITE_OK) {
        CortexErrCode classification = sqlite_exec_rc_classify(rc);
        const char *em = errmsg ? errmsg : sqlite3_errmsg(conn->handle);
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

int db_connection_prepare(DbConnection *conn, const char *sql, DbStatement **out)
{
    sqlite3_stmt *st = NULL;
    int rc;

    if (!conn || !conn->handle || !sql || !out) {
        CORTEX_SET_ERROR(CORTEX_ERR_INVALID_ARGUMENT, "sqlite:db_connection_prepare",
                         "missing connection handle, sql, or output pointer");
        return -1;
    }

    *out = NULL;
    rc = sqlite3_prepare_v2(conn->handle, sql, -1, &st, NULL);
    if (rc != SQLITE_OK) {
        CortexErrCode classification = sqlite_prepare_rc_classify(rc);
        const char *em = sqlite3_errmsg(conn->handle);
        sqlite_log_query_outcome(sql, 0, rc, em);
        CORTEX_SET_ERRORF(classification, "sqlite:db_connection_prepare",
                          "Prepare failed: %s (SQLite rc=%d)", em, rc);
        return -1;
    }

    *out = (DbStatement *)calloc(1, sizeof(DbStatement));
    if (!*out) {
        sqlite3_finalize(st);
        CORTEX_SET_ERROR(CORTEX_ERR_CORE_OOM, "sqlite:db_connection_prepare",
                         "Out of memory allocating DbStatement wrapper");
        return -1;
    }
    (*out)->stmt = st;
    cortex_clear_error();
    return 0;
}

int db_statement_step(DbStatement *stmt)
{
    int rc;

    if (!stmt || !stmt->stmt) {
        CORTEX_SET_ERROR(CORTEX_ERR_INVALID_ARGUMENT, "sqlite:db_statement_step",
                         "invalid statement");
        return -1;
    }

    rc = sqlite3_step(stmt->stmt);
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
        const char *em = sqlite3_errmsg(sqlite3_db_handle(stmt->stmt));
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

void db_statement_finalize(DbStatement *stmt)
{
    if (!stmt) {
        return;
    }
    if (stmt->stmt) {
        sqlite3_finalize(stmt->stmt);
        stmt->stmt = NULL;
    }
    free(stmt);
}

int db_statement_bind_int(DbStatement *stmt, int index, int value)
{
    int rc;

    if (!stmt || !stmt->stmt || index <= 0) {
        CORTEX_SET_ERROR(CORTEX_ERR_INVALID_ARGUMENT, "sqlite:db_statement_bind_int",
                         "invalid statement or bind index");
        return -1;
    }

    rc = sqlite3_bind_int(stmt->stmt, index, value);
    if (rc != SQLITE_OK) {
        CORTEX_SET_ERRORF(CORTEX_ERR_DB_PREPARE, "sqlite:db_statement_bind_int",
                          "bind failed: %s (SQLite rc=%d)",
                          sqlite3_errmsg(sqlite3_db_handle(stmt->stmt)), rc);
        return -1;
    }
    cortex_clear_error();
    return 0;
}

int db_statement_bind_text(DbStatement *stmt, int index, const char *value)
{
    int rc;

    if (!stmt || !stmt->stmt || index <= 0) {
        CORTEX_SET_ERROR(CORTEX_ERR_INVALID_ARGUMENT, "sqlite:db_statement_bind_text",
                         "invalid statement or bind index");
        return -1;
    }

    rc = sqlite3_bind_text(stmt->stmt, index, value ? value : "", -1, SQLITE_TRANSIENT);
    if (rc != SQLITE_OK) {
        CORTEX_SET_ERRORF(CORTEX_ERR_DB_PREPARE, "sqlite:db_statement_bind_text",
                          "bind failed: %s (SQLite rc=%d)",
                          sqlite3_errmsg(sqlite3_db_handle(stmt->stmt)), rc);
        return -1;
    }
    cortex_clear_error();
    return 0;
}

int db_statement_column_int(DbStatement *stmt, int col_index)
{
    if (!stmt || !stmt->stmt || col_index < 0) {
        return 0;
    }
    return sqlite3_column_int(stmt->stmt, col_index);
}

const char *db_statement_column_text(DbStatement *stmt, int col_index)
{
    if (!stmt || !stmt->stmt || col_index < 0) {
        return NULL;
    }
    return (const char *)sqlite3_column_text(stmt->stmt, col_index);
}
