#include "db_migration_generator.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

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

static int write_template_file(const char *path, const char *contents) {
    FILE *f = fopen(path, "w");
    if (!f) {
        return -1;
    }
    if (fputs(contents, f) < 0) {
        fclose(f);
        return -1;
    }
    if (fclose(f) != 0) {
        return -1;
    }
    return 0;
}

int db_generate_migration(int version, const char *slug) {
    char path[256];
    char contents[1024];
    time_t now;
    struct tm tm_now;
    char ts[64];

    if (version <= 0 || !slug || slug[0] == '\0') {
        return -1;
    }

    if (ensure_dir("db") != 0) {
        return -1;
    }
    if (ensure_dir("db/migrations") != 0) {
        return -1;
    }

    if (snprintf(path, sizeof(path), "db/migrations/%03d_%s.c", version, slug) < 0) {
        return -1;
    }

    now = time(NULL);
    tm_now = *localtime(&now);

    if (strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", &tm_now) == 0) {
        ts[0] = '\0';
    }

    if (snprintf(contents, sizeof(contents),
                 "/* Auto-generated migration: %s */\n"
                 "/* GeneratedAt: %s */\n\n"
                 "int up(void) {\n"
                 "    return 0;\n"
                 "}\n\n"
                 "int down(void) {\n"
                 "    return 0;\n"
                 "}\n",
                 slug, ts) < 0) {
        return -1;
    }

    return write_template_file(path, contents);
}

