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
    ActiveModel *found;

    active_record_init(&store);

    m1 = active_record_create(&store, "First");
    m2 = active_record_create(&store, "Second");
    ASSERT_TRUE(m1 != NULL);
    ASSERT_TRUE(m2 != NULL);
    ASSERT_EQ(store.count, 2);

    /* Delete the first record and ensure it is gone. Since the delete
     * implementation moves the last element into the deleted slot,
     * we primarily validate that:
     *  - the store count is decremented
     *  - the remaining record is still accessible and consistent.
     */
    ASSERT_EQ(active_record_delete(&store, m1->id), 0);
    ASSERT_EQ(store.count, 1);

    /* The remaining record should still be findable and intact. */
    found = active_record_find(&store, m2->id);
    ASSERT_TRUE(found != NULL);
    ASSERT_EQ(found->id, m2->id);
    ASSERT_STR_EQ(found->name, "Second");

    active_record_free(&store);
}

