#include "../test_assert.h"
#include "../../action/cx_context.h"

#include <string.h>

static void test_cx_init_defaults(void) {
    CxContext cx;
    cx_init(&cx);
    ASSERT_STR_EQ(cx.layout, "application");
    ASSERT_EQ(cx.buf_len, 0);
    ASSERT_EQ(cx.var_count, 0);
}

static void test_cx_set_get_string(void) {
    CxContext cx;
    cx_init(&cx);
    cx_set(&cx, "title", "Hello");
    ASSERT_STR_EQ(cx_get(&cx, "title"), "Hello");
    ASSERT_TRUE(cx_isset(&cx, "title"));
}

static void test_cx_set_int_and_get_int(void) {
    CxContext cx;
    cx_init(&cx);
    cx_set_int(&cx, "count", 42);
    ASSERT_EQ(cx_get_int(&cx, "count"), 42);
    ASSERT_STR_EQ(cx_get(&cx, "count"), "42");
}

static void test_cx_set_fmt(void) {
    CxContext cx;
    cx_init(&cx);
    cx_set_fmt(&cx, "label", "item-%d", 7);
    ASSERT_STR_EQ(cx_get(&cx, "label"), "item-7");
}

static void test_cx_write_and_writef(void) {
    CxContext cx;
    cx_init(&cx);
    cx_write(&cx, "ab", 2);
    cx_writef(&cx, "-%s", "cd");
    ASSERT_STR_EQ(cx.buf, "ab-cd");
}

static void test_cx_write_escaped(void) {
    CxContext cx;
    cx_init(&cx);
    cx_write_escaped(&cx, "<b>&\"'</b>");
    ASSERT_STR_EQ(cx.buf, "&lt;b&gt;&amp;&quot;&#39;&lt;/b&gt;");
}

static void test_cx_writef_escaped(void) {
    CxContext cx;
    cx_init(&cx);
    cx_writef_escaped(&cx, "%s", "<tag>");
    ASSERT_STR_EQ(cx.buf, "&lt;tag&gt;");
}

static void test_cx_yield_buffer(void) {
    CxContext cx;
    cx_init(&cx);
    cx_write(&cx, "inner", -1);
    memcpy(cx.yield_buf, cx.buf, (size_t)cx.buf_len);
    cx.yield_len = cx.buf_len;
    cx.yield_buf[cx.yield_len] = '\0';
    ASSERT_STR_EQ(cx_yield(&cx), "inner");
}

static void test_cx_set_layout_null(void) {
    CxContext cx;
    cx_init(&cx);
    cx_set_layout(&cx, NULL);
    ASSERT_NULL(cx.layout);
}

CT_SUITE_BEGIN(cx_context)
    CT_TEST(test_cx_init_defaults)
    CT_TEST(test_cx_set_get_string)
    CT_TEST(test_cx_set_int_and_get_int)
    CT_TEST(test_cx_set_fmt)
    CT_TEST(test_cx_write_and_writef)
    CT_TEST(test_cx_write_escaped)
    CT_TEST(test_cx_writef_escaped)
    CT_TEST(test_cx_yield_buffer)
    CT_TEST(test_cx_set_layout_null)
CT_SUITE_END()

void run_cx_context_tests(void) {
    CT_RUN_SUITE();
}
