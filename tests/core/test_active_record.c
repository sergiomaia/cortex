#include "../test_assert.h"
#include "../../core/active_record.h"

void test_active_record_create_save_find(void) {
    ActiveRecordStore store;
    ActiveModel *m1;
    ActiveModel *found;

    active_record_init(&store);

    m1 = active_record_create(&store, "Alice");
    ASSERT_TRUE(m1 != NULL);
    ASSERT_EQ(m1->id, 1);
    ASSERT_STR_EQ(m1->name, "Alice");
    ASSERT_EQ(store.count, 1);

    /* Modify the model and save it back. */
    m1->name = "Alice Updated";
    ASSERT_EQ(active_record_save(&store, m1), 0);

    found = active_record_find(&store, m1->id);
    ASSERT_TRUE(found != NULL);
    ASSERT_EQ(found->id, m1->id);
    ASSERT_STR_EQ(found->name, "Alice Updated");

    active_record_free(&store);
}

void test_active_record_delete_and_data_consistency(void) {
    ActiveRecordStore store;
    ActiveModel *m1;
    ActiveModel *m2;
    ActiveModel *m3;
    ActiveModel *found;
    int id1;
    int id2;
    int id3;

    active_record_init(&store);

    m1 = active_record_create(&store, "First");
    m2 = active_record_create(&store, "Second");
    m3 = active_record_create(&store, "Third");

    ASSERT_TRUE(m1 != NULL);
    ASSERT_TRUE(m2 != NULL);
    ASSERT_TRUE(m3 != NULL);
    ASSERT_EQ(store.count, 3);

    id1 = m1->id;
    id2 = m2->id;
    id3 = m3->id;
    ASSERT_EQ(id1, 1);
    ASSERT_EQ(id2, 2);
    ASSERT_EQ(id3, 3);

    /* Ensure each record can be retrieved correctly by its ID. */
    found = active_record_find(&store, id1);
    ASSERT_TRUE(found != NULL);
    ASSERT_EQ(found->id, id1);
    ASSERT_STR_EQ(found->name, "First");

    found = active_record_find(&store, id2);
    ASSERT_TRUE(found != NULL);
    ASSERT_EQ(found->id, id2);
    ASSERT_STR_EQ(found->name, "Second");

    found = active_record_find(&store, id3);
    ASSERT_TRUE(found != NULL);
    ASSERT_EQ(found->id, id3);
    ASSERT_STR_EQ(found->name, "Third");

    /* Delete one record and ensure it is gone, while other records remain
     * accessible and consistent. */
    ASSERT_EQ(active_record_delete(&store, id2), 0);
    ASSERT_EQ(store.count, 2);

    /* Deleted record should no longer be findable. */
    found = active_record_find(&store, id2);
    ASSERT_TRUE(found == NULL);

    /* Other records should still be findable and intact. */
    found = active_record_find(&store, id1);
    ASSERT_TRUE(found != NULL);
    ASSERT_EQ(found->id, id1);
    ASSERT_STR_EQ(found->name, "First");

    found = active_record_find(&store, id3);
    ASSERT_TRUE(found != NULL);
    ASSERT_EQ(found->id, id3);
    ASSERT_STR_EQ(found->name, "Third");

    active_record_free(&store);
}

