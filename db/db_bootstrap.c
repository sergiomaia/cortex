#include "db_bootstrap.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

    if (db_path_for_environment(NULL, path, sizeof(path)) != 0) {
        fprintf(stderr, "db: failed to resolve database path\n");
        return -1;
    }

    /* Apply pending migrations first (creates the file and schema_migrations). */
    if (db_migrate_default(path) != 0) {
        fprintf(stderr, "db: migrations failed\n");
        return -1;
    }

    return cortex_db_init();
}
