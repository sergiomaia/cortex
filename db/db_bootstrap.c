#include "db_bootstrap.h"

#include <stdio.h>

#include "../core/core_config.h"
#include "../core/core_secret.h"
#include "db_migrate.h"
#include "db_paths.h"
#include "db_pool.h"

static DbConnection *g_db;

DbConnection *cortex_db_connection(void) {
    return g_db;
}

int cortex_db_exec(const char *sql) {
    DbConnection *conn;

    if (!sql) {
        return -1;
    }
    conn = cortex_db_pool_acquire();
    if (!conn) {
        fprintf(stderr, "db: no database connection (cortex_db_init not called?)\n");
        return -1;
    }
    {
        int rc = db_connection_exec(conn, sql);
        cortex_db_pool_release(conn);
        return rc;
    }
}

int cortex_db_init(void) {
    int pool_size;
    DbConnection *conn;

    if (cortex_db_connection()) {
        return 0;
    }

    pool_size = core_config_get_int("db.pool_size", DB_POOL_DEFAULT_SIZE);

    if (cortex_db_pool_init(pool_size) != 0) {
        return -1;
    }

    conn = cortex_db_pool_acquire();
    if (!conn) {
        cortex_db_pool_shutdown();
        return -1;
    }
    g_db = conn;
    cortex_db_pool_release(conn);

    return 0;
}

void cortex_db_shutdown(void) {
    g_db = NULL;
    cortex_db_pool_shutdown();
}

int cortex_db_bootstrap(void) {
    char path[512];
    int has_pending = 0;

    if (cortex_secret_init() != 0) {
        fprintf(stderr, "secret bootstrap failed (set SECRET_KEY_BASE or config/secret.key for production)\n");
        return -1;
    }

    if (db_path_for_environment(NULL, path, sizeof(path)) != 0) {
        fprintf(stderr, "db: failed to resolve database path\n");
        return -1;
    }

    if (db_migrate_default_has_pending(path, &has_pending) != 0) {
        fprintf(stderr, "db: failed to check pending migrations\n");
        return -1;
    }
    if (has_pending) {
        fprintf(stderr, "db: pending migrations detected for '%s'\n", path);
        fprintf(stderr, "db: run `cortex db:migrate` before starting the server\n");
        return -1;
    }

    return cortex_db_init();
}
