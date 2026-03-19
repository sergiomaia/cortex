#include "db_create.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

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

int db_create(const char *storage_path) {
    const char *path = storage_path ? storage_path : "db/storage.json";
    FILE *f;

    if (!path || path[0] == '\0') {
        return -1;
    }

    /* Always ensure db/ exists for this scaffold. */
    if (ensure_dir("db") != 0) {
        return -1;
    }

    if (ensure_parent_dir(path) != 0) {
        return -1;
    }

    f = fopen(path, "w");
    if (!f) {
        return -1;
    }

    /* Initialize empty state. */
    if (fputs("[]\n", f) < 0) {
        fclose(f);
        return -1;
    }

    if (fclose(f) != 0) {
        return -1;
    }

    return 0;
}

