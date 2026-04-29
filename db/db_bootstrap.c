#include "db_bootstrap.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../core/core_secret.h"
#include "db_migrate.h"
#include "db_paths.h"

static DbConnection *g_db;

DbConnection *cortex_db_connection(void) {
    return g_db;
}

int cortex_db_exec(const char *sql) {
    if (!g_db) {
        fprintf(stderr, "db: no database connection (cortex_db_init not called?)\n");
        return -1;
    }
    return db_connection_exec(g_db, sql);
}

int cortex_db_init(void) {
    char path[512];

    if (g_db) {
        return 0;
    }

    if (db_path_for_environment(NULL, path, sizeof(path)) != 0) {
        fprintf(stderr, "db: failed to resolve database path\n");
        return -1;
    }

    if (db_connection_open(path, &g_db) != 0) {
        return -1;
    }
    return 0;
}

void cortex_db_shutdown(void) {
    if (!g_db) {
        return;
    }
    db_connection_close(g_db);
    g_db = NULL;
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
