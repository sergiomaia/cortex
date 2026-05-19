#include "../test_assert.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>

#ifndef CORTEX_SOURCE_ROOT
#define CORTEX_SOURCE_ROOT "."
#endif

static int run_compiler(const char *input, const char *output,
                        const char *fn, const char *view) {
    char cmd[1024];
    int n;

    n = snprintf(cmd, sizeof(cmd),
                 "%s/build/chtml_compile \"%s\" \"%s\" %s %s",
                 CORTEX_SOURCE_ROOT, input, output, fn, view);
    if (n < 0 || (size_t)n >= sizeof(cmd)) {
        return -1;
    }
    return system(cmd);
}

static void write_temp(const char *path, const char *body) {
    FILE *f = fopen(path, "wb");
    ASSERT_NOT_NULL(f);
    if (body && body[0]) {
        ASSERT_TRUE(fputs(body, f) >= 0);
    }
    fclose(f);
}

static int compiler_exit_status(const char *input, const char *output,
                                const char *fn, const char *view) {
    int rc = run_compiler(input, output, fn, view);
    if (rc == -1) {
        return -1;
    }
    if (WIFEXITED(rc)) {
        return WEXITSTATUS(rc);
    }
    return -1;
}

static void test_compile_valid_template(void) {
    (void)system("mkdir -p build/tests");
    const char *in = "build/tests/tmp_valid.chtml";
    const char *out = "build/tests/tmp_valid.chtml.c";
    FILE *f;

    write_temp(in, "<p><%= cx_get(cx, \"k\") %></p>\n");
    ASSERT_EQ(compiler_exit_status(in, out, "view_tmp_valid", "tmp/valid"), 0);

    f = fopen(out, "rb");
    ASSERT_NOT_NULL(f);
    {
        char buf[4096];
        size_t n = fread(buf, 1, sizeof(buf) - 1, f);
        buf[n] = '\0';
        ASSERT_TRUE(strstr(buf, "cx_write_escaped") != NULL);
        ASSERT_TRUE(strstr(buf, "CORTEX_VIEW(\"tmp/valid\"") != NULL);
    }
    fclose(f);
}

static void test_compile_unclosed_tag_fails(void) {
    (void)system("mkdir -p build/tests");
    const char *in = "build/tests/tmp_bad.chtml";
    const char *out = "build/tests/tmp_bad.chtml.c";

    write_temp(in, "<% if (1) { \n");
    ASSERT_NEQ(compiler_exit_status(in, out, "view_tmp_bad", "tmp/bad"), 0);
}

static void test_compile_literal_with_close_like_sequence(void) {
    (void)system("mkdir -p build/tests");
    const char *in = "build/tests/tmp_lit.chtml";
    const char *out = "build/tests/tmp_lit.chtml.c";
    FILE *f;

    write_temp(in, "<p>not a %> tag</p>\n");
    ASSERT_EQ(compiler_exit_status(in, out, "view_tmp_lit", "tmp/lit"), 0);
    f = fopen(out, "rb");
    ASSERT_NOT_NULL(f);
    {
        char buf[4096];
        size_t n = fread(buf, 1, sizeof(buf) - 1, f);
        buf[n] = '\0';
        ASSERT_TRUE(strstr(buf, "not a %> tag") != NULL);
    }
    fclose(f);
}

static void test_compile_crlf(void) {
    (void)system("mkdir -p build/tests");
    const char *in = "build/tests/tmp_crlf.chtml";
    const char *out = "build/tests/tmp_crlf.chtml.c";

    write_temp(in, "a\r\nb");
    ASSERT_EQ(compiler_exit_status(in, out, "view_tmp_crlf", "tmp/crlf"), 0);
}

CT_SUITE_BEGIN(chtml_compile)
    CT_TEST(test_compile_valid_template)
    CT_TEST(test_compile_unclosed_tag_fails)
    CT_TEST(test_compile_literal_with_close_like_sequence)
    CT_TEST(test_compile_crlf)
CT_SUITE_END()

void run_chtml_compile_tests(void) {
    CT_RUN_SUITE();
}
