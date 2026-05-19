#ifndef TEST_ASSERT_H
#define TEST_ASSERT_H

#include "cortex_test.h"

/* Legacy Cortex test helpers — routed through cortex_test.h counters. */

#define ASSERT_TRUE(cond)     CT_ASSERT(cond)
#define ASSERT_EQ(a, b)       CT_ASSERT_EQ((long)(a), (long)(b))
#define ASSERT_NEQ(a, b)      CT_ASSERT_NEQ((long)(a), (long)(b))
#define ASSERT_NULL(p)        CT_ASSERT_NULL(p)
#define ASSERT_NOT_NULL(p)    CT_ASSERT_NOT_NULL(p)
#define ASSERT_STR_EQ(a, b) do { \
        const char *__ct_sa = (a); \
        const char *__ct_sb = (b); \
        if (!__ct_sa) __ct_sa = ""; \
        if (!__ct_sb) __ct_sb = ""; \
        CT_ASSERT_STR_EQ(__ct_sa, __ct_sb); \
    } while (0)

#endif /* TEST_ASSERT_H */
