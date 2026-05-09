#include "../../db/db_connection.h"
#include "../../core/cortex_error.h"
#include "../cortex_test.h"

#include <stdlib.h>

static DbConnection *db;

static void setup(void) {
    db = NULL;
    CT_ASSERT_EQ(db_connection_open(":memory:", &db), 0);
    CT_ASSERT_NOT_NULL(db);
    CT_ASSERT_EQ(db_connection_exec(db,
        "CREATE TABLE posts ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  title TEXT NOT NULL,"
        "  body TEXT"
        ");"), 0);
}

static void teardown(void) {
    db_connection_close(db);
    db = NULL;
}

static void test_connect_memory(void) {
    DbConnection *c = NULL;
    CT_ASSERT_EQ(db_connection_open(":memory:", &c), 0);
    CT_ASSERT_NOT_NULL(c);
    db_connection_close(c);
}

static void test_exec_insert(void) {
    CT_ASSERT_EQ(db_connection_exec(db,
        "INSERT INTO posts (title, body) VALUES ('Hello', 'World');"), 0);
}

static void test_query_returns_rows(void) {
    DbStatement *stmt = NULL;
    int rows = 0;

    CT_ASSERT_EQ(db_connection_exec(db, "INSERT INTO posts (title) VALUES ('Test Post');"), 0);

    CT_ASSERT_EQ(db_connection_prepare(db, "SELECT * FROM posts;", &stmt), 0);
    CT_ASSERT_NOT_NULL(stmt);

    while (db_statement_step(stmt) == 1) {
        rows++;
    }
    CT_ASSERT_GT(rows, 0);

    db_statement_finalize(stmt);
}

static void test_prepared_bind_and_read(void) {
    DbStatement *stmt = NULL;
    int step_rc;

    CT_ASSERT_EQ(db_connection_prepare(db, "INSERT INTO posts (title, body) VALUES (?, ?);", &stmt), 0);
    CT_ASSERT_EQ(db_statement_bind_text(stmt, 1, "bound"), 0);
    CT_ASSERT_EQ(db_statement_bind_text(stmt, 2, "value"), 0);
    CT_ASSERT_EQ(db_statement_step(stmt), 0);
    db_statement_finalize(stmt);
    stmt = NULL;

    CT_ASSERT_EQ(db_connection_prepare(db, "SELECT title FROM posts WHERE body = ?;", &stmt), 0);
    CT_ASSERT_EQ(db_statement_bind_text(stmt, 1, "value"), 0);
    step_rc = db_statement_step(stmt);
    CT_ASSERT_EQ(step_rc, 1);
    CT_ASSERT_STR_EQ(db_statement_column_text(stmt, 0), "bound");
    db_statement_finalize(stmt);
}

static void test_connect_invalid_path(void) {
    DbConnection *bad = NULL;
    CT_ASSERT_NEQ(db_connection_open("/nonexistent_cortex_path_99/sub/db.sqlite3", &bad), 0);
    CT_ASSERT_NULL(bad);
    CT_ASSERT_EQ(cortex_has_error(), 1);
    CT_ASSERT_EQ(cortex_last_error()->code, CORTEX_ERR_DB_CONNECT);
    cortex_clear_error();
}

static void test_connect_rejects_empty_path(void) {
    DbConnection *bad = NULL;
    CT_ASSERT_NEQ(db_connection_open("", &bad), 0);
    CT_ASSERT_NULL(bad);
    CT_ASSERT_EQ(cortex_has_error(), 1);
    CT_ASSERT_EQ(cortex_last_error()->code, CORTEX_ERR_INVALID_ARGUMENT);
    cortex_clear_error();
}

static void test_close_null_safe(void) {
    db_connection_close(NULL);
}

CT_SUITE_BEGIN(db_connection)
    CT_TEST_WITH_HOOKS(test_connect_memory, NULL, NULL)
    CT_TEST_WITH_HOOKS(test_exec_insert, setup, teardown)
    CT_TEST_WITH_HOOKS(test_query_returns_rows, setup, teardown)
    CT_TEST_WITH_HOOKS(test_prepared_bind_and_read, setup, teardown)
    CT_TEST_WITH_HOOKS(test_connect_invalid_path, NULL, NULL)
    CT_TEST_WITH_HOOKS(test_connect_rejects_empty_path, NULL, NULL)
    CT_TEST_WITH_HOOKS(test_close_null_safe, NULL, NULL)
CT_SUITE_END()

void run_db_connection_tests(void) {
    CT_RUN_SUITE();
}
