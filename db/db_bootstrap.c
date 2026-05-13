#include "db_bootstrap.h"

#include "../core/core_config.h"
#include "../core/core_secret.h"
#include "../core/pulse.h"
#include "cortex_error.h"
#include "db_migrate.h"
#include "db_paths.h"
#include "db_pool.h"

#include <stdlib.h>
#include <string.h>

#ifdef CORTEX_HAVE_POSTGRES
#include "postgres/postgres_adapter.h"
#endif

static DbConnection *g_db;

static int cortex_env_is_production(void) {
    const char *env = getenv("CORE_ENV");
    return env && strcmp(env, "production") == 0;
}

int cortex_db_env_wants_postgresql(void)
{
    const char *db_url = getenv("DATABASE_URL");
    const char *ad = getenv("DB_ADAPTER");

    if (db_url && strncmp(db_url, "postgres", 8) == 0) {
        return 1;
    }
    if (ad && strcmp(ad, "postgresql") == 0) {
        return 1;
    }
    return 0;
}

DbConnection *cortex_db_connection(void)
{
    return g_db;
}

int cortex_db_exec(const char *sql)
{
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

int cortex_db_init(void)
{
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

void cortex_db_shutdown(void)
{
    g_db = NULL;
    cortex_db_pool_shutdown();
}

int cortex_db_bootstrap(void)
{
    char path[512];
    int has_pending = 0;
    int is_prod = cortex_env_is_production();

    if (cortex_secret_init() != 0) {
        CORTEX_SET_ERROR(
            CORTEX_ERR_UNKNOWN, "db:cortex_db_bootstrap",
            "secret bootstrap failed (set SECRET_KEY_BASE or config/secret.key for production)");
        return -1;
    }

    if (cortex_db_env_wants_postgresql()) {
#ifdef CORTEX_HAVE_POSTGRES
        DbConnection *probe = NULL;
        const char *db_url = getenv("DATABASE_URL");

        if (db_url && strncmp(db_url, "postgres", 8) == 0) {
            probe = postgres_connect(db_url);
        } else {
            probe = postgres_connect_from_env();
        }
        if (!probe) {
            cortex_err_print(cortex_last_error());
            return -1;
        }

        if (is_prod) {
            if (db_migrate_default_has_pending_conn(probe, &has_pending) != 0) {
                db_connection_close(probe);
                if (!cortex_has_error()) {
                    CORTEX_SET_ERROR(CORTEX_ERR_DB_EXEC, "db:cortex_db_bootstrap",
                                     "Unable to inspect PostgreSQL migration state");
                }
                return -1;
            }
            db_connection_close(probe);
            if (has_pending) {
                LOG_WARN("db.bootstrap",
                         "Pending migrations for PostgreSQL — run `cortex db:migrate` before relying on the schema");
            }
        } else {
            if (db_migrate_default_on_connection(probe) != 0) {
                db_connection_close(probe);
                if (!cortex_has_error()) {
                    CORTEX_SET_ERROR(CORTEX_ERR_DB_EXEC, "db:cortex_db_bootstrap",
                                     "Automatic PostgreSQL migration failed at boot");
                }
                return -1;
            }
            db_connection_close(probe);
        }
#else
        CORTEX_SET_ERROR(
            CORTEX_ERR_NOT_IMPLEMENTED, "db:cortex_db_bootstrap",
            "PostgreSQL was requested (DATABASE_URL / DB_ADAPTER) but this binary was built without libpq");
        return -1;
#endif
    } else {
        if (db_path_for_environment(NULL, path, sizeof(path)) != 0) {
            CORTEX_SET_ERROR(CORTEX_ERR_IO, "db:cortex_db_bootstrap",
                             "Failed to resolve database path for environment");
            return -1;
        }

        if (is_prod) {
            if (db_migrate_default_has_pending(path, &has_pending) != 0) {
                if (!cortex_has_error()) {
                    CORTEX_SET_ERRORF(CORTEX_ERR_DB_EXEC, "db:cortex_db_bootstrap",
                                      "Unable to inspect migration state for '%s'", path);
                }
                return -1;
            }
            if (has_pending) {
                LOG_WARN("db.bootstrap",
                         "Pending migrations for '%s' — run `cortex db:migrate` before relying on the schema",
                         path);
            }
        } else {
            if (db_migrate_default(NULL) != 0) {
                if (!cortex_has_error()) {
                    CORTEX_SET_ERRORF(CORTEX_ERR_DB_EXEC, "db:cortex_db_bootstrap",
                                      "Automatic migration failed for '%s'", path);
                }
                return -1;
            }
        }
    }

    return cortex_db_init();
}
