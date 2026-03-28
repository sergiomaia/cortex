#ifndef DB_PATHS_H
#define DB_PATHS_H

#include <stddef.h>

/* Resolve default SQLite path: db/<environment>.sqlite3
 * If `environment` is NULL or empty, uses CORE_ENV (default development). */
int db_path_for_environment(const char *environment, char *out, size_t out_size);

#endif /* DB_PATHS_H */
