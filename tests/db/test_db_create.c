#include "../test_assert.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../../db/db_create.h"

static int file_exists(const char *path) {
    FILE *f = fopen(path, "r");
    if (f == NULL) return 0;
    fclose(f);
    return 1;
}

static int dir_exists(const char *path) {
    struct stat st;
    if (stat(path, &st) != 0) return 0;
    return S_ISDIR(st.st_mode) ? 1 : 0;
}

void test_db_create_creates_db_folder_and_storage_file(void) {
    const char *storage = "db/test_db_create.sqlite3";

    /* Cleanup from previous runs. */
    if (file_exists(storage)) {
        remove(storage);
    }
    if (dir_exists("db")) {
        (void)rmdir("db");
    }

    ASSERT_EQ(db_create(storage), 0);
    ASSERT_TRUE(dir_exists("db"));
    ASSERT_TRUE(file_exists(storage));

    /* Cleanup. */
    if (file_exists(storage)) {
        remove(storage);
    }
    if (dir_exists("db")) {
        (void)rmdir("db");
    }
}

