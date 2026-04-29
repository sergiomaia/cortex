#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../../core/core_secret.h"
#include "../test_assert.h"

static const char *k_hex128 =
    "0123456789abcdef0123456789abcdef"
    "0123456789abcdef0123456789abcdef"
    "0123456789abcdef0123456789abcdef"
    "0123456789abcdef0123456789abcdef";

static int write_file_line(const char *path, const char *line) {
    FILE *f = fopen(path, "wb");
    size_t len;
    size_t nw;

    if (!f) {
        return -1;
    }
    len = strlen(line);
    nw = fwrite(line, 1, len, f);
    if (nw != len) {
        fclose(f);
        return -1;
    }
    fclose(f);
    return 0;
}

void test_secret_init_from_secret_key_base_env(void) {
    cortex_secret_reset_for_testing();
    unsetenv("SECRET_KEY_BASE");
    unsetenv("CORE_ENV");

    setenv("SECRET_KEY_BASE", k_hex128, 1);
    ASSERT_EQ(cortex_secret_init(), 0);
    ASSERT_TRUE(get_secret_key_base() != NULL);
    ASSERT_STR_EQ(get_secret_key_base(), k_hex128);

    unsetenv("SECRET_KEY_BASE");
    cortex_secret_reset_for_testing();
}

void test_secret_env_overrides_config_file(void) {
    char template[] = "/tmp/cortex_secret_test_XXXXXX";
    char cwd_buf[4096];
    char *tdir;
    char keypath[512];
    char config_dir[512];

    ASSERT_TRUE(getcwd(cwd_buf, sizeof(cwd_buf)) != NULL);

    tdir = mkdtemp(template);
    ASSERT_TRUE(tdir != NULL);

    snprintf(config_dir, sizeof(config_dir), "%s/config", tdir);
    snprintf(keypath, sizeof(keypath), "%s/secret.key", config_dir);

    ASSERT_EQ(chdir(tdir), 0);
    ASSERT_EQ(mkdir("config", 0755), 0);

    ASSERT_EQ(write_file_line(keypath, "fedcba9876543210fedcba9876543210fedcba9876543210fedcba9876543210"
                                         "fedcba9876543210fedcba9876543210fedcba9876543210fedcba9876543210"
                                         "fedcba9876543210fedcba9876543210fedcba9876543210fedcba9876543210"
                                         "fedcba9876543210fedcba9876543210fedcba9876543210fedcba9876543210\n"),
              0);

    cortex_secret_reset_for_testing();
    unsetenv("CORE_ENV");
    setenv("SECRET_KEY_BASE", k_hex128, 1);

    ASSERT_EQ(cortex_secret_init(), 0);
    ASSERT_STR_EQ(get_secret_key_base(), k_hex128);

    unsetenv("SECRET_KEY_BASE");
    cortex_secret_reset_for_testing();
    ASSERT_EQ(remove(keypath), 0);
    ASSERT_EQ(chdir(cwd_buf), 0);
    ASSERT_EQ(rmdir(config_dir), 0);
    ASSERT_EQ(rmdir(tdir), 0);
}

void test_secret_loads_from_config_file(void) {
    char template[] = "/tmp/cortex_secret_file_XXXXXX";
    char cwd_buf[4096];
    char *tdir;
    char keypath[512];
    char config_dir[512];

    ASSERT_TRUE(getcwd(cwd_buf, sizeof(cwd_buf)) != NULL);

    tdir = mkdtemp(template);
    ASSERT_TRUE(tdir != NULL);

    snprintf(config_dir, sizeof(config_dir), "%s/config", tdir);
    snprintf(keypath, sizeof(keypath), "%s/secret.key", config_dir);

    ASSERT_EQ(chdir(tdir), 0);
    ASSERT_EQ(mkdir("config", 0755), 0);

    cortex_secret_reset_for_testing();
    unsetenv("SECRET_KEY_BASE");
    unsetenv("CORE_ENV");

    ASSERT_EQ(write_file_line(keypath, k_hex128), 0);

    ASSERT_EQ(cortex_secret_init(), 0);
    ASSERT_TRUE(get_secret_key_base() != NULL);
    ASSERT_STR_EQ(get_secret_key_base(), k_hex128);

    cortex_secret_reset_for_testing();
    ASSERT_EQ(remove(keypath), 0);
    ASSERT_EQ(chdir(cwd_buf), 0);
    ASSERT_EQ(rmdir(config_dir), 0);
    ASSERT_EQ(rmdir(tdir), 0);
}

void test_secret_get_cached_stable_pointer(void) {
    const char *first;
    const char *second;

    cortex_secret_reset_for_testing();
    unsetenv("CORE_ENV");
    setenv("SECRET_KEY_BASE", k_hex128, 1);

    ASSERT_EQ(cortex_secret_init(), 0);
    first = get_secret_key_base();
    second = get_secret_key_base();
    ASSERT_TRUE(first != NULL);
    ASSERT_TRUE(first == second);

    unsetenv("SECRET_KEY_BASE");
    cortex_secret_reset_for_testing();
}

void test_secret_init_fails_in_production_without_key(void) {
    char template[] = "/tmp/cortex_secret_prod_XXXXXX";
    char cwd_buf[4096];
    char *tdir;

    ASSERT_TRUE(getcwd(cwd_buf, sizeof(cwd_buf)) != NULL);

    tdir = mkdtemp(template);
    ASSERT_TRUE(tdir != NULL);
    ASSERT_EQ(chdir(tdir), 0);

    cortex_secret_reset_for_testing();
    unsetenv("SECRET_KEY_BASE");
    setenv("CORE_ENV", "production", 1);

    ASSERT_EQ(cortex_secret_init(), -1);
    ASSERT_TRUE(get_secret_key_base() == NULL);

    unsetenv("CORE_ENV");
    cortex_secret_reset_for_testing();
    ASSERT_EQ(chdir(cwd_buf), 0);
    ASSERT_EQ(rmdir(tdir), 0);
}

void test_secret_production_accepts_env(void) {
    cortex_secret_reset_for_testing();
    setenv("CORE_ENV", "production", 1);
    setenv("SECRET_KEY_BASE", k_hex128, 1);

    ASSERT_EQ(cortex_secret_init(), 0);
    ASSERT_STR_EQ(get_secret_key_base(), k_hex128);

    unsetenv("CORE_ENV");
    unsetenv("SECRET_KEY_BASE");
    cortex_secret_reset_for_testing();
}
