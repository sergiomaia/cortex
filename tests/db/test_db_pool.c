#define _POSIX_C_SOURCE 200809L
#include "../test_assert.h"
#include "../cortex_test.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../../db/db_pool.h"

static void pool_remove_file_if_exists(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) {
        return;
    }
    fclose(f);
    (void)remove(path);
}

void test_db_pool_initialization_and_clamped_size(void) {
    DbPool pool;
    const char *path = "db/test_pool_init.sqlite3";

    pool_remove_file_if_exists(path);

    ASSERT_EQ(db_pool_init(&pool, path, DB_POOL_MAX_SIZE + 10), 0);
    ASSERT_EQ(pool.size, DB_POOL_MAX_SIZE);
    db_pool_shutdown(&pool);

    ASSERT_EQ(db_pool_init(&pool, path, 0), 0);
    ASSERT_EQ(pool.size, DB_POOL_DEFAULT_SIZE);
    db_pool_shutdown(&pool);

    pool_remove_file_if_exists(path);
}

void test_db_pool_acquire_release_and_reuse(void) {
    DbPool pool;
    const char *path = "db/test_pool_reuse.sqlite3";
    DbConnection *a;
    DbConnection *b;

    pool_remove_file_if_exists(path);

    ASSERT_EQ(db_pool_init(&pool, path, 2), 0);
    a = db_pool_acquire(&pool);
    ASSERT_TRUE(a != NULL);
    db_pool_release(&pool, a);

    b = db_pool_acquire(&pool);
    ASSERT_TRUE(b != NULL);
    ASSERT_TRUE(a == b);
    db_pool_release(&pool, b);

    db_pool_shutdown(&pool);
    pool_remove_file_if_exists(path);
}

void test_db_pool_acquire_multiple_connections(void) {
    DbPool pool;
    const char *path = "db/test_pool_multi.sqlite3";
    DbConnection *c1;
    DbConnection *c2;
    DbConnection *c3;

    pool_remove_file_if_exists(path);

    ASSERT_EQ(db_pool_init(&pool, path, 3), 0);
    c1 = db_pool_acquire(&pool);
    c2 = db_pool_acquire(&pool);
    c3 = db_pool_acquire(&pool);

    ASSERT_TRUE(c1 != NULL);
    ASSERT_TRUE(c2 != NULL);
    ASSERT_TRUE(c3 != NULL);
    ASSERT_TRUE(c1 != c2);
    ASSERT_TRUE(c2 != c3);
    ASSERT_TRUE(c1 != c3);

    db_pool_release(&pool, c1);
    db_pool_release(&pool, c2);
    db_pool_release(&pool, c3);
    db_pool_shutdown(&pool);
    pool_remove_file_if_exists(path);
}

typedef struct {
    DbPool *pool;
    DbConnection *acquired;
    int acquired_flag;
} PoolThreadArgs;

static void *pool_blocking_worker(void *arg) {
    PoolThreadArgs *args = (PoolThreadArgs *)arg;
    args->acquired = db_pool_acquire(args->pool);
    args->acquired_flag = args->acquired != NULL ? 1 : 0;
    if (args->acquired) {
        db_pool_release(args->pool, args->acquired);
    }
    return NULL;
}

static void pool_sleep_ms(int ms) {
    struct timespec ts;
    if (ms <= 0) {
        return;
    }
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (long)(ms % 1000) * 1000000L;
    (void)nanosleep(&ts, NULL);
}

void test_db_pool_blocking_behavior_when_exhausted(void) {
    DbPool pool;
    const char *path = "db/test_pool_blocking.sqlite3";
    DbConnection *held;
    pthread_t tid;
    PoolThreadArgs args;

    pool_remove_file_if_exists(path);
    ASSERT_EQ(db_pool_init(&pool, path, 1), 0);

    held = db_pool_acquire(&pool);
    ASSERT_TRUE(held != NULL);

    args.pool = &pool;
    args.acquired = NULL;
    args.acquired_flag = 0;

    ASSERT_EQ(pthread_create(&tid, NULL, pool_blocking_worker, &args), 0);

    pool_sleep_ms(100);
    ASSERT_EQ(args.acquired_flag, 0);

    db_pool_release(&pool, held);

    ASSERT_EQ(pthread_join(tid, NULL), 0);
    ASSERT_EQ(args.acquired_flag, 1);

    db_pool_shutdown(&pool);
    pool_remove_file_if_exists(path);
}

void test_db_pool_applies_wal_mode(void) {
    DbPool pool;
    const char *path = "db/test_pool_wal.sqlite3";
    DbConnection *conn;
    DbStatement *stmt = NULL;
    int step_rc;
    const char *journal_mode;

    pool_remove_file_if_exists(path);

    ASSERT_EQ(db_pool_init(&pool, path, 2), 0);
    conn = db_pool_acquire(&pool);
    ASSERT_TRUE(conn != NULL);

    ASSERT_EQ(db_connection_prepare(conn, "PRAGMA journal_mode;", &stmt), 0);
    step_rc = db_statement_step(stmt);
    ASSERT_EQ(step_rc, 1);
    journal_mode = db_statement_column_text(stmt, 0);
    ASSERT_TRUE(journal_mode != NULL);
    ASSERT_STR_EQ(journal_mode, "wal");

    db_statement_finalize(stmt);
    db_pool_release(&pool, conn);
    db_pool_shutdown(&pool);
    pool_remove_file_if_exists(path);
}

void test_db_pool_shutdown_is_idempotent(void) {
    DbPool pool;
    const char *path = "db/test_pool_shutdown.sqlite3";

    pool_remove_file_if_exists(path);
    ASSERT_EQ(db_pool_init(&pool, path, 2), 0);
    db_pool_shutdown(&pool);
    db_pool_shutdown(&pool);
    pool_remove_file_if_exists(path);
}

void test_db_pool_global_api_acquire_release(void) {
    DbConnection *conn;

    ASSERT_EQ(cortex_db_pool_init(2), 0);
    conn = cortex_db_pool_acquire();
    ASSERT_TRUE(conn != NULL);
    cortex_db_pool_release(conn);
    cortex_db_pool_shutdown();
    cortex_db_pool_shutdown();
}

CT_SUITE_BEGIN(db_pool)
    CT_TEST(test_db_pool_initialization_and_clamped_size)
    CT_TEST(test_db_pool_acquire_release_and_reuse)
    CT_TEST(test_db_pool_acquire_multiple_connections)
    CT_TEST(test_db_pool_blocking_behavior_when_exhausted)
    CT_TEST(test_db_pool_applies_wal_mode)
    CT_TEST(test_db_pool_shutdown_is_idempotent)
    CT_TEST(test_db_pool_global_api_acquire_release)
CT_SUITE_END()

void run_db_pool_tests(void) {
    CT_RUN_SUITE();
}
