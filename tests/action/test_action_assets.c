#include "../test_assert.h"

#include "../../action/action_assets.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static int file_contains_substr(const char *path, const char *needle) {
    FILE *f;
    char buf[8192];
    size_t n;

    if (!path || !needle) {
        return 0;
    }
    f = fopen(path, "rb");
    if (!f) {
        return 0;
    }
    n = fread(buf, 1, sizeof(buf) - 1, f);
    fclose(f);
    buf[n] = '\0';
    return strstr(buf, needle) != NULL;
}

static int write_text_file(const char *path, const char *text) {
    FILE *f = fopen(path, "w");
    if (!f) {
        return -1;
    }
    if (fputs(text, f) < 0) {
        fclose(f);
        return -1;
    }
    return fclose(f) == 0 ? 0 : -1;
}

void test_action_assets_serves_stylesheet_from_app_assets(void) {
    char cwd[4096];
    char tmp[] = "/tmp/cortex_tassetsXXXXXX";
    char outpath[512];
    char cmd[512];

    ASSERT_TRUE(getcwd(cwd, sizeof(cwd)) != NULL);
    ASSERT_TRUE(mkdtemp(tmp) != NULL);
    ASSERT_EQ(chdir(tmp), 0);

    ASSERT_EQ(mkdir("app", 0755), 0);
    ASSERT_EQ(mkdir("app/assets", 0755), 0);
    ASSERT_EQ(mkdir("app/assets/stylesheets", 0755), 0);
    ASSERT_EQ(write_text_file("app/assets/stylesheets/application.css", "body{color:#abc}"), 0);

    snprintf(outpath, sizeof(outpath), "%s/http_out.bin", tmp);
    {
        int fd = open(outpath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        ASSERT_TRUE(fd >= 0);
        ASSERT_EQ(action_assets_serve_static_path("/assets/stylesheets/application.css", fd), 0);
        close(fd);
    }

    ASSERT_TRUE(file_contains_substr(outpath, "text/css"));
    ASSERT_TRUE(file_contains_substr(outpath, "body{color:#abc}"));

    ASSERT_EQ(chdir(cwd), 0);
    snprintf(cmd, sizeof(cmd), "rm -rf \"%s\"", tmp);
    (void)system(cmd);
}

void test_action_assets_public_directory_over_app_assets(void) {
    char cwd[4096];
    char tmp[] = "/tmp/cortex_tassets2XXXXXX";
    char outpath[512];
    char cmd[512];

    ASSERT_TRUE(getcwd(cwd, sizeof(cwd)) != NULL);
    ASSERT_TRUE(mkdtemp(tmp) != NULL);
    ASSERT_EQ(chdir(tmp), 0);

    ASSERT_EQ(mkdir("public", 0755), 0);
    ASSERT_EQ(mkdir("public/assets", 0755), 0);
    ASSERT_EQ(mkdir("public/assets/stylesheets", 0755), 0);
    ASSERT_EQ(write_text_file("public/assets/stylesheets/application.css", "from_public{}"), 0);

    ASSERT_EQ(mkdir("app", 0755), 0);
    ASSERT_EQ(mkdir("app/assets", 0755), 0);
    ASSERT_EQ(mkdir("app/assets/stylesheets", 0755), 0);
    ASSERT_EQ(write_text_file("app/assets/stylesheets/application.css", "from_app{}"), 0);

    snprintf(outpath, sizeof(outpath), "%s/http_out2.bin", tmp);
    {
        int fd = open(outpath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        ASSERT_TRUE(fd >= 0);
        ASSERT_EQ(action_assets_serve_static_path("/assets/stylesheets/application.css", fd), 0);
        close(fd);
    }

    ASSERT_TRUE(file_contains_substr(outpath, "from_public{}"));
    ASSERT_TRUE(file_contains_substr(outpath, "from_app{}") == 0);

    ASSERT_EQ(chdir(cwd), 0);
    snprintf(cmd, sizeof(cmd), "rm -rf \"%s\"", tmp);
    (void)system(cmd);
}

void test_action_assets_rejects_path_traversal(void) {
    int fd = open("/dev/null", O_WRONLY);
    ASSERT_TRUE(fd >= 0);
    ASSERT_EQ(action_assets_serve_static_path("/assets/../etc/passwd", fd), -1);
    close(fd);
}
