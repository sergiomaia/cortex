#ifndef DB_CONNECTION_H
#define DB_CONNECTION_H

#include <stddef.h>

typedef enum {
    CORTEX_DB_BACKEND_SQLITE = 0,
    CORTEX_DB_BACKEND_POSTGRESQL = 1,
} CortexDbBackend;

/*
 * Engine-agnostic database facade. Backends register via db_connection_open (SQLite)
 * or postgres_connect / connection pool when PostgreSQL is enabled and selected.
 */

typedef struct DbConnection DbConnection;
typedef struct DbStatement DbStatement;

CortexDbBackend db_connection_backend(const DbConnection *conn);

int db_connection_open(const char *path, DbConnection **out);
void db_connection_close(DbConnection *conn);

/* Execute one or more SQL statements (no result rows consumed here). */
int db_connection_exec(DbConnection *conn, const char *sql);

int db_connection_prepare(DbConnection *conn, const char *sql, DbStatement **out);
/* Returns 1 if a row is available, 0 if done, -1 on error. */
int db_statement_step(DbStatement *stmt);
void db_statement_finalize(DbStatement *stmt);

int db_statement_bind_int(DbStatement *stmt, int index, int value);
int db_statement_bind_text(DbStatement *stmt, int index, const char *value);
int db_statement_column_int(DbStatement *stmt, int col_index);

/* Text column; pointer is valid until the next step/finalize on this statement. */
const char *db_statement_column_text(DbStatement *stmt, int col_index);

#endif /* DB_CONNECTION_H */
