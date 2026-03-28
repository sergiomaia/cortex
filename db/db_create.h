#ifndef DB_CREATE_H
#define DB_CREATE_H

/* Create the local database storage for cortex.
 *
 * If `storage_path` is NULL or empty, uses the default SQLite file for the
 * current environment (db/<CORE_ENV>.sqlite3, default development).
 *
 * If `storage_path` ends with `.json`, creates the legacy JSON migration store.
 * Otherwise creates a SQLite database file at the given path.
 *
 * Returns 0 on success, non-zero on error.
 */
int db_create(const char *storage_path);

/* Create db/development.sqlite3 under an app directory (for `cortex new`). */
int db_create_default_in_project(const char *project_root);

#endif /* DB_CREATE_H */

