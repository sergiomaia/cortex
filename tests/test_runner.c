#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int test_count = 0;
static int test_failures = 0;

#define ASSERT_TRUE(cond)                                                       \
    do {                                                                        \
        ++test_count;                                                           \
        if (!(cond)) {                                                          \
            ++test_failures;                                                   \
            printf("[FAIL] %s:%d: ASSERT_TRUE(%s)\n", __FILE__, __LINE__, #cond); \
        } else {                                                                \
            printf("[PASS] %s:%d: ASSERT_TRUE(%s)\n", __FILE__, __LINE__, #cond); \
        }                                                                       \
    } while (0)

#define ASSERT_EQ(a, b)                                                         \
    do {                                                                        \
        ++test_count;                                                           \
        long _va = (long)(a);                                                   \
        long _vb = (long)(b);                                                   \
        if (_va != _vb) {                                                       \
            ++test_failures;                                                   \
            printf("[FAIL] %s:%d: ASSERT_EQ(%s, %s) => %ld != %ld\n",           \
                   __FILE__, __LINE__, #a, #b, _va, _vb);                       \
        } else {                                                                \
            printf("[PASS] %s:%d: ASSERT_EQ(%s, %s)\n",                         \
                   __FILE__, __LINE__, #a, #b);                                 \
        }                                                                       \
    } while (0)

#define ASSERT_STR_EQ(a, b)                                                     \
    do {                                                                        \
        ++test_count;                                                           \
        const char *_sa = (a);                                                  \
        const char *_sb = (b);                                                  \
        if (!_sa) _sa = "";                                                     \
        if (!_sb) _sb = "";                                                     \
        if (strcmp(_sa, _sb) != 0) {                                            \
            ++test_failures;                                                   \
            printf("[FAIL] %s:%d: ASSERT_STR_EQ(%s, %s) => \"%s\" != \"%s\"\n", \
                   __FILE__, __LINE__, #a, #b, _sa, _sb);                       \
        } else {                                                                \
            printf("[PASS] %s:%d: ASSERT_STR_EQ(%s, %s)\n",                     \
                   __FILE__, __LINE__, #a, #b);                                 \
        }                                                                       \
    } while (0)

static void run_all_tests(void) {
    /* Placeholder: add real tests here using the ASSERT_* macros. */
    ASSERT_TRUE(1);
    ASSERT_EQ(1, 1);
    ASSERT_STR_EQ("ok", "ok");
}

int main(void) {
    run_all_tests();

    printf("\nTotal tests: %d\n", test_count);
    printf("Failures   : %d\n", test_failures);

    if (test_failures > 0) {
        return 1;
    }
    return 0;
}

