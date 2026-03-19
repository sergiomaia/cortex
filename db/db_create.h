#ifndef DB_CREATE_H
#define DB_CREATE_H

/* Create the local database storage for cortex.
 *
 * Expected behavior:
 *   - Ensure the `db/` directory exists.
 *   - Create/overwrite the storage file with an empty initial state.
 *
 * Returns 0 on success, non-zero on error.
 */
int db_create(const char *storage_path);

#endif /* DB_CREATE_H */

