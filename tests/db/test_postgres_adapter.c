#define _POSIX_C_SOURCE 200809L

#include "../../db/db_bootstrap.h"
#include "../../db/db_connection.h"
#include "../cortex_test.h"

#include <stdlib.h>

#ifdef CORTEX_HAVE_POSTGRES
#include "../../db/postgres/postgres_adapter.h"
#endif

static void test_backend_sqlite_is_default_for_memory(void)
{
    DbConnection *c = NULL;

    CT_ASSERT_EQ(db_connection_open(":memory:", &c), 0);
    CT_ASSERT_EQ(db_connection_backend(c), CORTEX_DB_BACKEND_SQLITE);
    db_connection_close(c);
}

/*
 * Mutates process environment briefly; the test runner is single-threaded.
 */
static void test_env_selects_postgresql(void)
{
    const char *old_ad = getenv("DB_ADAPTER");
    const char *old_url = getenv("DATABASE_URL");
    char buf_ad[512];
    char buf_url[512];

    buf_ad[0] = '\0';
    buf_url[0] = '\0';
    if (old_ad) {
        (void)snprintf(buf_ad, sizeof(buf_ad), "%s", old_ad);
    }
    if (old_url) {
        (void)snprintf(buf_url, sizeof(buf_url), "%s", old_url);
    }

    (void)unsetenv("DATABASE_URL");
    (void)unsetenv("DB_ADAPTER");
    CT_ASSERT_EQ(cortex_db_env_wants_postgresql(), 0);

    (void)setenv("DB_ADAPTER", "postgresql", 1);
    CT_ASSERT_EQ(cortex_db_env_wants_postgresql(), 1);
    (void)unsetenv("DB_ADAPTER");

    (void)setenv("DATABASE_URL", "postgresql://user:pass@localhost:5432/app", 1);
    CT_ASSERT_EQ(cortex_db_env_wants_postgresql(), 1);
    (void)unsetenv("DATABASE_URL");

    (void)setenv("DATABASE_URL", "postgres://localhost/db", 1);
    CT_ASSERT_EQ(cortex_db_env_wants_postgresql(), 1);
    (void)unsetenv("DATABASE_URL");

    if (buf_ad[0] != '\0') {
        (void)setenv("DB_ADAPTER", buf_ad, 1);
    } else {
        (void)unsetenv("DB_ADAPTER");
    }
    if (buf_url[0] != '\0') {
        (void)setenv("DATABASE_URL", buf_url, 1);
    } else {
        (void)unsetenv("DATABASE_URL");
    }
}

#ifdef CORTEX_HAVE_POSTGRES
static void test_pg_adapter_roundtrip_optional(void)
{
    DbConnection *conn;

    if (!getenv("CORTEX_TEST_PG")) {
        return;
    }

    conn = postgres_connect_from_env();
    if (!conn) {
        /* Server not reachable — skip without failing CI. */
        return;
    }

    CT_ASSERT_EQ(db_connection_backend(conn), CORTEX_DB_BACKEND_POSTGRESQL);
    CT_ASSERT_EQ(db_connection_exec(conn,
                                   "CREATE TEMP TABLE IF NOT EXISTS cortex_pg_adapter_test "
                                   "(id SERIAL PRIMARY KEY, title TEXT NOT NULL);"),
                 0);

    {
        DbStatement *st = NULL;
        CT_ASSERT_EQ(db_connection_prepare(conn,
                                           "INSERT INTO cortex_pg_adapter_test (title) VALUES (?);",
                                           &st),
                     0);
        CT_ASSERT_EQ(db_statement_bind_text(st, 1, "hello"), 0);
        CT_ASSERT_EQ(db_statement_step(st), 0);
        db_statement_finalize(st);
    }

    {
        DbStatement *st = NULL;
        int step_rc;

        CT_ASSERT_EQ(db_connection_prepare(conn,
                                           "SELECT title FROM cortex_pg_adapter_test WHERE title = ?;",
                                           &st),
                     0);
        CT_ASSERT_EQ(db_statement_bind_text(st, 1, "hello"), 0);
        step_rc = db_statement_step(st);
        CT_ASSERT_EQ(step_rc, 1);
        CT_ASSERT_STR_EQ(db_statement_column_text(st, 0), "hello");
        db_statement_finalize(st);
    }

    db_connection_close(conn);
}
#endif

CT_SUITE_BEGIN(postgres_adapter)
    CT_TEST(test_backend_sqlite_is_default_for_memory)
    CT_TEST(test_env_selects_postgresql)
#ifdef CORTEX_HAVE_POSTGRES
    CT_TEST(test_pg_adapter_roundtrip_optional)
#endif
CT_SUITE_END()

void run_postgres_adapter_tests(void)
{
    CT_RUN_SUITE();
}
