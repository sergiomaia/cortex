#include "db_bootstrap.h"

#include "../core/core_config.h"
#include "../core/core_secret.h"
#include "cortex_error.h"
#include "db_migrate.h"
#include "db_paths.h"
#include "db_pool.h"

static DbConnection *g_db;

DbConnection *cortex_db_connection(void) {
    return g_db;
}

int cortex_db_exec(const char *sql) {
    DbConnection *conn;
    int rc;

    if (!sql) {
        CORTEX_SET_ERROR(CORTEX_ERR_INVALID_ARGUMENT, "db:cortex_db_exec",
                         "SQL string is NULL");
        return -1;
    }
    conn = cortex_db_pool_acquire();
    if (!conn) {
        CORTEX_SET_ERROR(CORTEX_ERR_DB_CONNECT, "db:cortex_db_exec",
                         "No database connection (initialize the pool via cortex_db_init first)");
        return -1;
    }
    rc = db_connection_exec(conn, sql);
    cortex_db_pool_release(conn);
    /* db_connection_exec clears the thread-local error slot on success. */
    return rc;
}

int cortex_db_init(void) {
    int pool_size;
    DbConnection *conn;

    if (cortex_db_connection()) {
        cortex_clear_error();
        return 0;
    }

    pool_size = core_config_get_int("db.pool_size", DB_POOL_DEFAULT_SIZE);

    if (cortex_db_pool_init(pool_size) != 0) {
        if (!cortex_has_error()) {
            CORTEX_SET_ERROR(CORTEX_ERR_DB_CONNECT, "db:cortex_db_init",
                             "Connection pool initialization failed");
        }
        return -1;
    }

    conn = cortex_db_pool_acquire();
    if (!conn) {
        cortex_db_pool_shutdown();
        CORTEX_SET_ERROR(CORTEX_ERR_DB_CONNECT, "db:cortex_db_init",
                         "Unable to acquire a connection after pool init");
        return -1;
    }
    g_db = conn;
    cortex_db_pool_release(conn);

    cortex_clear_error();
    return 0;
}

void cortex_db_shutdown(void) {
    g_db = NULL;
    cortex_db_pool_shutdown();
}

int cortex_db_bootstrap(void) {
    char path[512];
    int has_pending = 0;

    if (core_secret_init() != 0) {
        CORTEX_SET_ERROR(
            CORTEX_ERR_UNKNOWN, "db:cortex_db_bootstrap",
            "secret bootstrap failed (set SECRET_KEY_BASE or config/secret.key for production)");
        return -1;
    }

    if (db_path_for_environment(NULL, path, sizeof(path)) != 0) {
        CORTEX_SET_ERROR(CORTEX_ERR_IO, "db:cortex_db_bootstrap",
                         "Failed to resolve database path for environment");
        return -1;
    }

    if (db_migrate_default_has_pending(path, &has_pending) != 0) {
        if (!cortex_has_error()) {
            CORTEX_SET_ERRORF(CORTEX_ERR_DB_EXEC, "db:cortex_db_bootstrap",
                              "Unable to inspect migration state for '%s'", path);
        }
        return -1;
    }
    if (has_pending) {
        CORTEX_SET_ERRORF(
            CORTEX_ERR_DB_MIGRATION_PENDING, "db:cortex_db_bootstrap",
            "Pending migrations for '%s' - run `cortex db:migrate` before starting the server",
            path);
        return -1;
    }

    return cortex_db_init();
}
