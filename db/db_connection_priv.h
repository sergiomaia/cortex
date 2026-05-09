/*
 * Internal database connection layout shared by SQLite and PostgreSQL adapters.
 * Not a public API — include only from db_connection.c and backend adapters.
 */
#ifndef DB_CONNECTION_PRIV_H
#define DB_CONNECTION_PRIV_H

#include "db_connection.h"

typedef struct DbConnectionOps DbConnectionOps;

struct DbConnectionOps {
    void (*conn_close)(void *ctx);
    int (*exec)(void *ctx, const char *sql);
    int (*prepare)(void *ctx, const char *sql, DbStatement *stmt);
    int (*step)(DbStatement *stmt);
    void (*finalize_stmt)(DbStatement *stmt);
    int (*bind_int)(DbStatement *stmt, int index, int value);
    int (*bind_text)(DbStatement *stmt, int index, const char *value);
    int (*column_int)(DbStatement *stmt, int col_index);
    const char *(*column_text)(DbStatement *stmt, int col_index);
};

struct DbConnection {
    CortexDbBackend backend;
    const DbConnectionOps *ops;
    void *ctx;
};

struct DbStatement {
    DbConnection *conn;
    void *stmt_ctx;
};

#endif /* DB_CONNECTION_PRIV_H */
