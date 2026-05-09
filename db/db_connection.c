#include "db_connection_priv.h"

#include "cortex_error.h"

#include "sqlite/sqlite_adapter.h"

#include <stdlib.h>

CortexDbBackend db_connection_backend(const DbConnection *conn)
{
    if (!conn) {
        return CORTEX_DB_BACKEND_SQLITE;
    }
    return conn->backend;
}

int db_connection_open(const char *path, DbConnection **out)
{
    return cortex_sqlite_connection_open(path, out);
}

void db_connection_close(DbConnection *conn)
{
    if (!conn || !conn->ops || !conn->ops->conn_close) {
        return;
    }
    conn->ops->conn_close(conn->ctx);
    free(conn);
}

int db_connection_exec(DbConnection *conn, const char *sql)
{
    if (!conn || !conn->ops || !conn->ops->exec) {
        CORTEX_SET_ERROR(CORTEX_ERR_INVALID_ARGUMENT, "db:db_connection_exec",
                         "invalid connection");
        return -1;
    }
    return conn->ops->exec(conn->ctx, sql);
}

int db_connection_prepare(DbConnection *conn, const char *sql, DbStatement **out)
{
    DbStatement *stmt;

    if (!conn || !conn->ops || !conn->ops->prepare || !sql || !out) {
        CORTEX_SET_ERROR(CORTEX_ERR_INVALID_ARGUMENT, "db:db_connection_prepare",
                         "invalid arguments");
        return -1;
    }

    *out = NULL;
    stmt = (DbStatement *)calloc(1, sizeof(DbStatement));
    if (!stmt) {
        CORTEX_SET_ERROR(CORTEX_ERR_CORE_OOM, "db:db_connection_prepare",
                         "Out of memory allocating DbStatement");
        return -1;
    }
    stmt->conn = conn;

    if (conn->ops->prepare(conn->ctx, sql, stmt) != 0) {
        free(stmt);
        return -1;
    }

    *out = stmt;
    cortex_clear_error();
    return 0;
}

int db_statement_step(DbStatement *stmt)
{
    if (!stmt || !stmt->conn || !stmt->conn->ops || !stmt->conn->ops->step) {
        CORTEX_SET_ERROR(CORTEX_ERR_INVALID_ARGUMENT, "db:db_statement_step",
                         "invalid statement");
        return -1;
    }
    return stmt->conn->ops->step(stmt);
}

void db_statement_finalize(DbStatement *stmt)
{
    if (!stmt || !stmt->conn || !stmt->conn->ops || !stmt->conn->ops->finalize_stmt) {
        free(stmt);
        return;
    }
    stmt->conn->ops->finalize_stmt(stmt);
    free(stmt);
}

int db_statement_bind_int(DbStatement *stmt, int index, int value)
{
    if (!stmt || !stmt->conn || !stmt->conn->ops || !stmt->conn->ops->bind_int) {
        CORTEX_SET_ERROR(CORTEX_ERR_INVALID_ARGUMENT, "db:db_statement_bind_int",
                         "invalid statement");
        return -1;
    }
    return stmt->conn->ops->bind_int(stmt, index, value);
}

int db_statement_bind_text(DbStatement *stmt, int index, const char *value)
{
    if (!stmt || !stmt->conn || !stmt->conn->ops || !stmt->conn->ops->bind_text) {
        CORTEX_SET_ERROR(CORTEX_ERR_INVALID_ARGUMENT, "db:db_statement_bind_text",
                         "invalid statement");
        return -1;
    }
    return stmt->conn->ops->bind_text(stmt, index, value);
}

int db_statement_column_int(DbStatement *stmt, int col_index)
{
    if (!stmt || !stmt->conn || !stmt->conn->ops || !stmt->conn->ops->column_int) {
        return 0;
    }
    return stmt->conn->ops->column_int(stmt, col_index);
}

const char *db_statement_column_text(DbStatement *stmt, int col_index)
{
    if (!stmt || !stmt->conn || !stmt->conn->ops || !stmt->conn->ops->column_text) {
        return NULL;
    }
    return stmt->conn->ops->column_text(stmt, col_index);
}
