#ifndef TEST_ASSERT_H
#define TEST_ASSERT_H

#include <stdio.h>
#include <string.h>

extern int test_count;
extern int test_failures;

#define ASSERT_TRUE(cond)                                                       \
    do {                                                                        \
        ++test_count;                                                           \
        if (!(cond)) {                                                          \
            ++test_failures;                                                    \
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
            ++test_failures;                                                    \
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
            ++test_failures;                                                    \
            printf("[FAIL] %s:%d: ASSERT_STR_EQ(%s, %s) => \"%s\" != \"%s\"\n", \
                   __FILE__, __LINE__, #a, #b, _sa, _sb);                       \
        } else {                                                                \
            printf("[PASS] %s:%d: ASSERT_STR_EQ(%s, %s)\n",                     \
                   __FILE__, __LINE__, #a, #b);                                 \
        }                                                                       \
    } while (0)

#endif /* TEST_ASSERT_H */

