#include "../test_assert.h"
#include "../../action/action_view.h"
#include "../../action/cx_context.h"

#include <stdio.h>
#include <string.h>

#define HTML_SZ (64 * 1024)

static void test_render_simple_view(void) {
    CxContext cx;
    char html[HTML_SZ];
    int rc;

    cx_init(&cx);
    cx_set_layout(&cx, NULL);
    cx_set(&cx, "name", "Cortex");
    rc = action_view_render("simple/index", &cx, html, (int)sizeof(html));
    ASSERT_EQ(rc, 0);
    ASSERT_TRUE(strstr(html, "Hello Cortex") != NULL);
}

static void test_render_escape(void) {
    CxContext cx;
    char html[HTML_SZ];

    cx_init(&cx);
    cx_set_layout(&cx, NULL);
    cx_set(&cx, "unsafe", "<script>");
    ASSERT_EQ(action_view_render("escape/index", &cx, html, (int)sizeof(html)), 0);
    ASSERT_TRUE(strstr(html, "&lt;script&gt;") != NULL);
    ASSERT_NULL(strstr(html, "<script>"));
}

static void test_render_raw(void) {
    CxContext cx;
    char html[HTML_SZ];

    cx_init(&cx);
    cx_set_layout(&cx, NULL);
    cx_set(&cx, "html", "<strong>x</strong>");
    ASSERT_EQ(action_view_render("raw/index", &cx, html, (int)sizeof(html)), 0);
    ASSERT_TRUE(strstr(html, "<strong>x</strong>") != NULL);
}

static void test_render_code_branch(void) {
    CxContext cx;
    char html[HTML_SZ];

    cx_init(&cx);
    cx_set_layout(&cx, NULL);
    cx_set_int(&cx, "n", 1);
    ASSERT_EQ(action_view_render("code/index", &cx, html, (int)sizeof(html)), 0);
    ASSERT_TRUE(strstr(html, "positive") != NULL);

    cx_init(&cx);
    cx_set_layout(&cx, NULL);
    cx_set_int(&cx, "n", 0);
    ASSERT_EQ(action_view_render("code/index", &cx, html, (int)sizeof(html)), 0);
    ASSERT_TRUE(strstr(html, "zero") != NULL);
}

static void test_render_comment_stripped(void) {
    CxContext cx;
    char html[HTML_SZ];

    cx_init(&cx);
    cx_set_layout(&cx, NULL);
    ASSERT_EQ(action_view_render("comment/index", &cx, html, (int)sizeof(html)), 0);
    ASSERT_TRUE(strstr(html, "visible") != NULL);
    ASSERT_NULL(strstr(html, "ignored"));
}

static void test_render_literals_percent_lt(void) {
    CxContext cx;
    char html[HTML_SZ];

    cx_init(&cx);
    cx_set_layout(&cx, NULL);
    cx_set(&cx, "x", "!");
    ASSERT_EQ(action_view_render("literals/index", &cx, html, (int)sizeof(html)), 0);
    ASSERT_TRUE(strstr(html, "100%") != NULL);
    ASSERT_TRUE(strstr(html, "&lt;tag&gt;") != NULL);
}

static void test_render_empty_template(void) {
    CxContext cx;
    char html[HTML_SZ];

    cx_init(&cx);
    cx_set_layout(&cx, NULL);
    ASSERT_EQ(action_view_render("empty/index", &cx, html, (int)sizeof(html)), 0);
    ASSERT_STR_EQ(html, "");
}

static void test_render_missing_view(void) {
    CxContext cx;
    char html[HTML_SZ];

    cx_init(&cx);
    ASSERT_EQ(action_view_render("does/not/exist", &cx, html, (int)sizeof(html)), -2);
}

static void test_render_missing_layout(void) {
    CxContext cx;
    char html[HTML_SZ];

    cx_init(&cx);
    cx_set(&cx, "name", "X");
    cx_set_layout(&cx, "missing_layout");
    ASSERT_EQ(action_view_render("simple/index", &cx, html, (int)sizeof(html)), -5);
}

static void test_render_with_layout_yield(void) {
    CxContext cx;
    char html[HTML_SZ];

    cx_init(&cx);
    cx_set_layout(&cx, "minimal");
    cx_set(&cx, "name", "Inner");
    ASSERT_EQ(action_view_render("simple/index", &cx, html, (int)sizeof(html)), 0);
    ASSERT_TRUE(strstr(html, "<wrap>") != NULL);
    ASSERT_TRUE(strstr(html, "Hello Inner") != NULL);
    ASSERT_TRUE(strstr(html, "</wrap>") != NULL);
}

static void test_render_consecutive(void) {
    CxContext cx;
    char html[HTML_SZ];

    cx_init(&cx);
    cx_set_layout(&cx, NULL);
    cx_set(&cx, "name", "A");
    ASSERT_EQ(action_view_render("simple/index", &cx, html, (int)sizeof(html)), 0);
    ASSERT_TRUE(strstr(html, "A") != NULL);

    cx_set(&cx, "name", "B");
    ASSERT_EQ(action_view_render("simple/index", &cx, html, (int)sizeof(html)), 0);
    ASSERT_TRUE(strstr(html, "B") != NULL);
    ASSERT_NULL(strstr(html, "A"));
}

static void test_render_nested_code(void) {
    CxContext cx;
    char html[HTML_SZ];

    cx_init(&cx);
    cx_set_layout(&cx, NULL);
    cx_set(&cx, "inner", "deep");
    ASSERT_EQ(action_view_render("nested/index", &cx, html, (int)sizeof(html)), 0);
    ASSERT_TRUE(strstr(html, ">deep<") != NULL);
}

static void test_app_posts_index_registered(void) {
    CxContext cx;
    char html[HTML_SZ];

    cx_init(&cx);
    cx_set_layout(&cx, NULL);
    cx_set_int(&cx, "posts_count", 0);
    cx_set(&cx, "posts_list", "");
    ASSERT_EQ(action_view_render("posts/index", &cx, html, (int)sizeof(html)), 0);
    ASSERT_TRUE(strstr(html, "Nenhum post encontrado") != NULL);
}

CT_SUITE_BEGIN(action_chtml)
    CT_TEST(test_render_simple_view)
    CT_TEST(test_render_escape)
    CT_TEST(test_render_raw)
    CT_TEST(test_render_code_branch)
    CT_TEST(test_render_comment_stripped)
    CT_TEST(test_render_literals_percent_lt)
    CT_TEST(test_render_empty_template)
    CT_TEST(test_render_missing_view)
    CT_TEST(test_render_missing_layout)
    CT_TEST(test_render_with_layout_yield)
    CT_TEST(test_render_consecutive)
    CT_TEST(test_render_nested_code)
    CT_TEST(test_app_posts_index_registered)
CT_SUITE_END()

void run_action_chtml_tests(void) {
    CT_RUN_SUITE();
}
