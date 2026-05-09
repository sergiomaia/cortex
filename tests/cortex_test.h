#ifndef CORTEX_TEST_H
#define CORTEX_TEST_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* ──────────────────────────────────────────
   Global suite counters

   Stored in a single translation unit (tests/cortex_test.c) so all test
   files share totals when compiled and linked separately.
   ────────────────────────────────────────── */
extern int _ct_pass;
extern int _ct_fail;
extern int _ct_total;

/* ──────────────────────────────────────────
   ANSI colors (disable with -DNO_COLOR)
   ────────────────────────────────────────── */
#ifndef NO_COLOR
  #define CT_GREEN  "\033[0;32m"
  #define CT_RED    "\033[0;31m"
  #define CT_YELLOW "\033[0;33m"
  #define CT_RESET  "\033[0m"
#else
  #define CT_GREEN  ""
  #define CT_RED    ""
  #define CT_YELLOW ""
  #define CT_RESET  ""
#endif

/* ──────────────────────────────────────────
   Thread-local errors (cortex_error.h)

   Callers can assert on `cortex_has_error()`, `cortex_last_error()->code`, and
   `cortex_last_error()->source` after failed operations. Use `cortex_clear_error()`
   between cases when a clean assertion surface is required.

   Assertions
   ────────────────────────────────────────── */
#define CT_ASSERT(expr) do { \
    _ct_total++; \
    if (expr) { \
        _ct_pass++; \
    } else { \
        _ct_fail++; \
        fprintf(stderr, CT_RED "  ✗ FAIL" CT_RESET \
                " %s:%d → %s\n", __FILE__, __LINE__, #expr); \
    } \
} while(0)

#define CT_ASSERT_EQ(a, b)    CT_ASSERT((a) == (b))
#define CT_ASSERT_NEQ(a, b)   CT_ASSERT((a) != (b))
#define CT_ASSERT_NULL(p)     CT_ASSERT((p) == NULL)
#define CT_ASSERT_NOT_NULL(p) CT_ASSERT((p) != NULL)
#define CT_ASSERT_GT(a, b)    CT_ASSERT((a) > (b))
#define CT_ASSERT_LT(a, b)    CT_ASSERT((a) < (b))

#define CT_ASSERT_STR_EQ(a, b) do { \
    _ct_total++; \
    if (strcmp((a), (b)) == 0) { \
        _ct_pass++; \
    } else { \
        _ct_fail++; \
        fprintf(stderr, CT_RED "  ✗ FAIL" CT_RESET \
                " %s:%d → \"%s\" != \"%s\"\n", \
                __FILE__, __LINE__, (a), (b)); \
    } \
} while(0)

/* ──────────────────────────────────────────
   Test definitions and execution
   ────────────────────────────────────────── */
typedef void (*CortexTestFn)(void);

typedef struct {
    const char   *name;
    CortexTestFn  fn;
    CortexTestFn  setup;
    CortexTestFn  teardown;
} CortexTest;

#define CT_SUITE_BEGIN(suite_name) \
    static const char *_ct_suite = #suite_name; \
    static CortexTest _ct_tests[] = {

#define CT_TEST(fn) { #fn, fn, NULL, NULL },
#define CT_TEST_WITH_HOOKS(fn, setup, teardown) { #fn, fn, setup, teardown },

#define CT_SUITE_END() \
    { NULL, NULL, NULL, NULL } };

#define CT_RUN_SUITE() do { \
    printf(CT_YELLOW "\n▶ Suite: %s\n" CT_RESET, _ct_suite); \
    for (int _i = 0; _ct_tests[_i].fn != NULL; _i++) { \
        int _before = _ct_fail; \
        if (_ct_tests[_i].setup) _ct_tests[_i].setup(); \
        _ct_tests[_i].fn(); \
        if (_ct_tests[_i].teardown) _ct_tests[_i].teardown(); \
        if (_ct_fail == _before) \
            printf(CT_GREEN "  ✓" CT_RESET " %s\n", _ct_tests[_i].name); \
    } \
} while(0)

/* ──────────────────────────────────────────
   Final report
   ────────────────────────────────────────── */
#define CT_REPORT() do { \
    printf("\n────────────────────────────────\n"); \
    printf("Tests: %d  " CT_GREEN "✓ %d" CT_RESET \
           "  " CT_RED "✗ %d" CT_RESET "\n", \
           _ct_total, _ct_pass, _ct_fail); \
    if (_ct_fail == 0) \
        printf(CT_GREEN "All tests passed.\n" CT_RESET); \
    else \
        printf(CT_RED "%d test(s) failed.\n" CT_RESET, _ct_fail); \
} while(0)

#define CT_EXIT_CODE() (_ct_fail > 0 ? 1 : 0)

#endif /* CORTEX_TEST_H */
