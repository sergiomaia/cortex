#include "../test_assert.h"
#include "../../core/active_record.h"
#include "../../core/active_query.h"

void test_active_query_where_filters_by_name(void) {
    ActiveRecordStore store;
    ActiveQuery query;
    ActiveModel *m1;
    ActiveModel *m2;
    ActiveModel *m3;
    ActiveModel **results;
    int count = 0;

    active_record_init(&store);

    m1 = active_record_create(&store, "Alice");
    m2 = active_record_create(&store, "Bob");
    m3 = active_record_create(&store, "Alice");

    ASSERT_TRUE(m1 != NULL);
    ASSERT_TRUE(m2 != NULL);
    ASSERT_TRUE(m3 != NULL);
    ASSERT_EQ(store.count, 3);

    active_query_init(&query, &store);
    active_query_where(&query, "name", "Alice");

    results = active_query_execute(&query, &count);

    ASSERT_TRUE(results != NULL);
    ASSERT_EQ(count, 2);
    ASSERT_TRUE(results[0] != NULL);
    ASSERT_TRUE(results[1] != NULL);
    ASSERT_STR_EQ(results[0]->name, "Alice");
    ASSERT_STR_EQ(results[1]->name, "Alice");

    free(results);
    active_record_free(&store);
}

void test_active_query_limit_caps_results(void) {
    ActiveRecordStore store;
    ActiveQuery query;
    ActiveModel *m1;
    ActiveModel *m2;
    ActiveModel *m3;
    ActiveModel **results;
    int count = 0;

    active_record_init(&store);

    m1 = active_record_create(&store, "Alice");
    m2 = active_record_create(&store, "Bob");
    m3 = active_record_create(&store, "Charlie");

    ASSERT_TRUE(m1 != NULL);
    ASSERT_TRUE(m2 != NULL);
    ASSERT_TRUE(m3 != NULL);
    ASSERT_EQ(store.count, 3);

    active_query_init(&query, &store);
    active_query_limit(&query, 2);

    results = active_query_execute(&query, &count);

    ASSERT_TRUE(results != NULL);
    ASSERT_EQ(count, 2);

    free(results);
    active_record_free(&store);
}

void test_active_query_where_and_limit_chaining(void) {
    ActiveRecordStore store;
    ActiveQuery query;
    ActiveModel *m1;
    ActiveModel *m2;
    ActiveModel *m3;
    ActiveModel **results;
    int count = 0;
    ActiveQuery *qptr;

    active_record_init(&store);

    m1 = active_record_create(&store, "Alice");
    m2 = active_record_create(&store, "Bob");
    m3 = active_record_create(&store, "Alice");

    ASSERT_TRUE(m1 != NULL);
    ASSERT_TRUE(m2 != NULL);
    ASSERT_TRUE(m3 != NULL);
    ASSERT_EQ(store.count, 3);

    active_query_init(&query, &store);

    /* Verify basic chaining returns the same query pointer. */
    qptr = active_query_where(&query, "name", "Alice");
    ASSERT_TRUE(qptr == &query);
    qptr = active_query_limit(qptr, 1);
    ASSERT_TRUE(qptr == &query);

    /* Support C-style equivalent of where(...).limit(...). */
    results = active_query_execute(
        active_query_limit(
            active_query_where(&query, "name", "Alice"),
            1),
        &count);

    ASSERT_TRUE(results != NULL);
    ASSERT_EQ(count, 1);
    ASSERT_TRUE(results[0] != NULL);
    ASSERT_STR_EQ(results[0]->name, "Alice");

    free(results);
    active_record_free(&store);
}

void test_active_query_where_filters_by_custom_field(void) {
    ActiveRecordStore store;
    ActiveQuery query;
    ActiveModel *m1;
    ActiveModel *m2;
    ActiveModel **results;
    int count = 0;

    active_record_init(&store);

    m1 = active_record_create(&store, "Alice");
    m2 = active_record_create(&store, "Bob");
    ASSERT_TRUE(m1 != NULL);
    ASSERT_TRUE(m2 != NULL);

    ASSERT_EQ(active_model_set_field(m1, "role", "admin"), 0);
    ASSERT_EQ(active_model_set_field(m2, "role", "user"), 0);

    active_query_init(&query, &store);
    active_query_where(&query, "role", "admin");

    results = active_query_execute(&query, &count);
    ASSERT_TRUE(results != NULL);
    ASSERT_EQ(count, 1);
    ASSERT_TRUE(results[0] != NULL);
    ASSERT_STR_EQ(results[0]->name, "Alice");

    free(results);
    active_record_free(&store);
}

void test_active_query_execute_returns_empty_array_when_no_match(void) {
    ActiveRecordStore store;
    ActiveQuery query;
    ActiveModel **results;
    int count = 0;

    active_record_init(&store);
    ASSERT_TRUE(active_record_create(&store, "Alice") != NULL);
    ASSERT_TRUE(active_record_create(&store, "Bob") != NULL);

    active_query_init(&query, &store);
    active_query_where(&query, "name", "DoesNotExist");

    results = active_query_execute(&query, &count);
    ASSERT_TRUE(results != NULL);
    ASSERT_EQ(count, 0);

    free(results);
    active_record_free(&store);
}

void test_active_query_where_filters_by_id(void) {
    ActiveRecordStore store;
    ActiveQuery query;
    ActiveModel *m1;
    ActiveModel *m2;
    ActiveModel **results;
    int count = 0;

    active_record_init(&store);
    m1 = active_record_create(&store, "First");
    m2 = active_record_create(&store, "Second");
    ASSERT_TRUE(m1 != NULL);
    ASSERT_TRUE(m2 != NULL);

    active_query_init(&query, &store);
    active_query_where(&query, "id", "2");

    results = active_query_execute(&query, &count);
    ASSERT_TRUE(results != NULL);
    ASSERT_EQ(count, 1);
    ASSERT_TRUE(results[0] != NULL);
    ASSERT_EQ(results[0]->id, 2);
    ASSERT_STR_EQ(results[0]->name, "Second");

    free(results);
    active_record_free(&store);
}

