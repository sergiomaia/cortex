#ifndef SQLITE_ADAPTER_H
#define SQLITE_ADAPTER_H

#include "../db_connection.h"

/* Open a SQLite database at `path` and wrap it as a DbConnection. */
int cortex_sqlite_connection_open(const char *path, DbConnection **out);

#endif /* SQLITE_ADAPTER_H */
