#include "forge.h"

#include "../db/db_bootstrap.h"
#include "../db/db_connection.h"
#include "../db/db_create.h"
#include "../db/db_migrate.h"
#include "../db/db_migration_runner.h"
#include "../db/db_paths.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef CORTEX_HAVE_POSTGRES
#include "../db/postgres/postgres_adapter.h"
#endif

static int db_path_file_exists(const char *path) {
    FILE *f;

    if (!path || path[0] == '\0') {
        return 0;
    }
    f = fopen(path, "rb");
    if (!f) {
        return 0;
    }
    fclose(f);
    return 1;
}

static int open_env_db(DbConnection **out) {
    char path[512];
    const char *db_url;

    if (!out) {
        return -1;
    }
    *out = NULL;

    if (cortex_db_env_wants_postgresql()) {
#ifdef CORTEX_HAVE_POSTGRES
        db_url = getenv("DATABASE_URL");
        if (db_url && strncmp(db_url, "postgres", 8) == 0) {
            *out = postgres_connect(db_url);
        } else {
            *out = postgres_connect_from_env();
        }
        return *out ? 0 : -1;
#else
        fprintf(stderr, "db: PostgreSQL requested but this cortex binary was built without libpq\n");
        return -1;
#endif
    }

    if (db_path_for_environment(NULL, path, sizeof(path)) != 0) {
        fprintf(stderr, "db: could not resolve SQLite path for current environment\n");
        return -1;
    }
    if (!db_path_file_exists(path)) {
        if (db_create(NULL) != 0) {
            fprintf(stderr, "db: could not create database file\n");
            return -1;
        }
    }
    return db_connection_open(path, out);
}

int forge_db_migrate(void) {
    DbConnection *conn = NULL;
    int rc;

    if (open_env_db(&conn) != 0) {
        return -1;
    }
    rc = db_migrate_default_on_connection(conn);
    db_connection_close(conn);
    return rc;
}

int forge_db_rollback(int steps) {
    DbConnection *conn = NULL;
    MigrationResult r;

    if (steps <= 0) {
        fprintf(stderr, "db:rollback: step count must be >= 1\n");
        return -1;
    }
    if (open_env_db(&conn) != 0) {
        return -1;
    }
    r = db_migration_rollback_sql_files(conn, steps);
    db_connection_close(conn);
    if (r.errors > 0) {
        return -1;
    }
    return 0;
}

int forge_db_status(void) {
    DbConnection *conn = NULL;
    MigrationResult r;

    if (open_env_db(&conn) != 0) {
        return -1;
    }
    r = db_migration_status_sql_files(conn, stdout);
    db_connection_close(conn);
    if (r.errors > 0) {
        return -1;
    }
    return 0;
}

int forge_generate_migration(const char *name) {
    return db_migration_generate(name);
}
