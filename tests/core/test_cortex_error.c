#include <pthread.h>
#include <string.h>

#include "../../core/cortex_error.h"
#include "../cortex_test.h"

typedef struct {
    CortexErrCode code;
    char source[CORTEX_ERR_SRC_MAX];
} ErrorCapsule;

static ErrorCapsule g_capsules[2];

static void *test_error_tls_worker(void *arg)
{
    int which = *(int *)arg;

    cortex_clear_error();
    if (which == 0) {
        CORTEX_SET_ERROR(CORTEX_ERR_DB_CONNECT, "test:worker_a", "thread A scoped failure");
    } else {
        CORTEX_SET_ERROR(CORTEX_ERR_ACTIVE_VALIDATION, "test:worker_b", "thread B scoped failure");
    }

    /* Snapshot before returning so TLS remains valid throughout the pthread lifetime. */
    g_capsules[which].code = cortex_last_error()->code;
    (void)strncpy(g_capsules[which].source, cortex_last_error()->source, sizeof(g_capsules[which].source) - 1);
    g_capsules[which].source[sizeof(g_capsules[which].source) - 1] = '\0';
    return NULL;
}

void test_cortex_error_has_and_clear_helpers(void)
{
    cortex_clear_error();
    CT_ASSERT_EQ(cortex_has_error(), 0);

    CORTEX_SET_ERROR(CORTEX_ERR_IO, "suite:cortex_helpers", "example io failure");

    CT_ASSERT_EQ(cortex_has_error(), 1);
    CT_ASSERT_EQ(cortex_last_error()->code, CORTEX_ERR_IO);
    CT_ASSERT_STR_EQ(cortex_last_error()->source, "suite:cortex_helpers");

    cortex_clear_error();
    CT_ASSERT_EQ(cortex_has_error(), 0);
}

void test_cortex_error_http_mapping(void)
{
    CT_ASSERT_EQ(cortex_err_to_http_status(CORTEX_ERR_ACTION_BAD_REQUEST), 400);
    CT_ASSERT_EQ(cortex_err_to_http_status(CORTEX_ERR_ACTION_UNAUTHORIZED), 401);
    CT_ASSERT_EQ(cortex_err_to_http_status(CORTEX_ERR_GUARD_AUTH_FAILED), 401);
    CT_ASSERT_EQ(cortex_err_to_http_status(CORTEX_ERR_ACTION_FORBIDDEN), 403);
    CT_ASSERT_EQ(cortex_err_to_http_status(CORTEX_ERR_ACTION_NOT_FOUND), 404);
    CT_ASSERT_EQ(cortex_err_to_http_status(CORTEX_ERR_ACTIVE_NOT_FOUND), 404);
    CT_ASSERT_EQ(cortex_err_to_http_status(CORTEX_ERR_ACTIVE_VALIDATION), 422);
    CT_ASSERT_EQ(cortex_err_to_http_status(CORTEX_ERR_DB_CONNECT), 500);
}

void test_cortex_err_code_slug_strings(void)
{
    CT_ASSERT_STR_EQ(cortex_err_code_str(CORTEX_ERR_ACTIVE_VALIDATION), "ACTIVE_VALIDATION");
    CT_ASSERT_STR_EQ(cortex_err_code_str(CORTEX_ERR_DB_CONNECT), "DB_CONNECT");
}

void test_cortex_error_thread_local_isolation(void)
{
    pthread_t t0;
    pthread_t t1;
    int ids[2] = {0, 1};

    cortex_clear_error();

    CT_ASSERT_EQ(pthread_create(&t0, NULL, test_error_tls_worker, &ids[0]), 0);
    CT_ASSERT_EQ(pthread_create(&t1, NULL, test_error_tls_worker, &ids[1]), 0);
    CT_ASSERT_EQ(pthread_join(t0, NULL), 0);
    CT_ASSERT_EQ(pthread_join(t1, NULL), 0);

    CT_ASSERT_NEQ(g_capsules[0].code, g_capsules[1].code);
    CT_ASSERT_EQ(g_capsules[0].code, CORTEX_ERR_DB_CONNECT);
    CT_ASSERT_EQ(g_capsules[1].code, CORTEX_ERR_ACTIVE_VALIDATION);
    CT_ASSERT_STR_EQ(g_capsules[0].source, "test:worker_a");
}

void test_chained_sources_propagate_via_fields(void)
{
    cortex_clear_error();

    /* Simulate layering: SQLite layer fills first, upstream refines wording. */
    CORTEX_SET_ERROR(CORTEX_ERR_DB_EXEC, "db:sqlite", "PRIMARY KEY violated");
    CORTEX_SET_ERRORF(CORTEX_ERR_DB_EXEC, "active:active_save", "%s via ORM persistence",
                      cortex_last_error()->message);

    CT_ASSERT_EQ(cortex_last_error()->code, CORTEX_ERR_DB_EXEC);
    CT_ASSERT_STR_EQ(cortex_last_error()->source, "active:active_save");
}

CT_SUITE_BEGIN(cortex_error)
    CT_TEST(test_cortex_error_has_and_clear_helpers)
    CT_TEST(test_cortex_error_http_mapping)
    CT_TEST(test_cortex_err_code_slug_strings)
    CT_TEST(test_cortex_error_thread_local_isolation)
    CT_TEST(test_chained_sources_propagate_via_fields)
CT_SUITE_END()

void run_cortex_error_tests(void)
{
    CT_RUN_SUITE();
}
