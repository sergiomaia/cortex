#include "../test_assert.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/stat.h>

#include "../../db/db_migration_generator.h"

static int file_exists(const char *path) {
    FILE *f = fopen(path, "r");
    if (f == NULL) return 0;
    fclose(f);
    return 1;
}

static void remove_if_exists(const char *path) {
    if (file_exists(path)) {
        remove(path);
    }
}

static int file_contains(const char *path, const char *substring) {
    FILE *f;
    char buf[512];
    int found = 0;

    if (!substring || substring[0] == '\0') return 1;

    f = fopen(path, "r");
    if (!f) return 0;

    while (fgets(buf, (int)sizeof(buf), f) != NULL) {
        if (strstr(buf, substring) != NULL) {
            found = 1;
            break;
        }
    }

    fclose(f);
    return found;
}

void test_db_migration_generator_creates_timestamped_file_with_up_down(void) {
    const int version = 1;
    const char *slug = "create_users";
    const char *path = "db/migrations/001_create_users.c";

    remove_if_exists(path);

    ASSERT_EQ(db_generate_migration(version, slug), 0);
    ASSERT_TRUE(file_exists(path));

    /* Timestamp marker should be present. */
    ASSERT_TRUE(file_contains(path, "GeneratedAt:"));

    /* Provide up/down functions. */
    ASSERT_TRUE(file_contains(path, "int up(void)"));
    ASSERT_TRUE(file_contains(path, "int down(void)"));

    remove_if_exists(path);
}

