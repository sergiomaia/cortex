#include "db_create.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "db_connection.h"
#include "db_paths.h"

static int ensure_dir(const char *path) {
    if (!path || path[0] == '\0') {
        return -1;
    }

    if (mkdir(path, 0755) == -1) {
        if (errno == EEXIST) {
            return 0;
        }
        return -1;
    }
    return 0;
}

/* Best-effort ensure that the parent directory of `storage_path` exists. */
static int ensure_parent_dir(const char *storage_path) {
    char parent[256];
    const char *slash;
    size_t len;

    if (!storage_path) {
        return -1;
    }

    slash = strrchr(storage_path, '/');
    if (!slash) {
        /* No parent directory. */
        return 0;
    }

    len = (size_t)(slash - storage_path);
    if (len == 0 || len >= sizeof(parent)) {
        return -1;
    }

    memcpy(parent, storage_path, len);
    parent[len] = '\0';

    return ensure_dir(parent);
}

static int path_is_json_storage(const char *path) {
    static const char suf[] = ".json";
    size_t len;
    size_t slen;

    if (!path) {
        return 0;
    }
    len = strlen(path);
    slen = sizeof(suf) - 1;
    if (len < slen) {
        return 0;
    }
    return strcmp(path + (len - slen), suf) == 0;
}

static int db_create_sqlite_file(const char *path) {
    DbConnection *conn = NULL;

    if (db_connection_open(path, &conn) != 0) {
        return -1;
    }
    db_connection_close(conn);
    return 0;
}

int db_create(const char *storage_path) {
    char buf[512];
    const char *path = storage_path;
    FILE *f;

    if (!path || path[0] == '\0') {
        if (db_path_for_environment(NULL, buf, sizeof(buf)) != 0) {
            return -1;
        }
        path = buf;
    }

    /* Always ensure db/ exists for this scaffold. */
    if (ensure_dir("db") != 0) {
        return -1;
    }

    if (ensure_parent_dir(path) != 0) {
        return -1;
    }

    if (!path_is_json_storage(path)) {
        return db_create_sqlite_file(path);
    }

    f = fopen(path, "w");
    if (!f) {
        return -1;
    }

    /* Initialize empty state (legacy JSON migration store). */
    if (fputs("[]\n", f) < 0) {
        fclose(f);
        return -1;
    }

    if (fclose(f) != 0) {
        return -1;
    }

    return 0;
}

int db_create_default_in_project(const char *project_root) {
    char path[640];
    char dbdir[640];

    if (!project_root || project_root[0] == '\0') {
        return -1;
    }

    if (snprintf(dbdir, sizeof(dbdir), "%s/db", project_root) >= (int)sizeof(dbdir)) {
        return -1;
    }
    if (ensure_dir(dbdir) != 0) {
        return -1;
    }

    if (snprintf(path, sizeof(path), "%s/db/development.sqlite3", project_root) >= (int)sizeof(path)) {
        return -1;
    }

    return db_create_sqlite_file(path);
}
