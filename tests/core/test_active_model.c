#include "../test_assert.h"
#include "../../core/active_model.h"

void test_active_model_init_sets_id_and_starts_empty(void) {
    ActiveModel model;

    active_model_init(&model, 42);

    ASSERT_EQ(model.id, 42);
    ASSERT_TRUE(model.name == NULL);
    ASSERT_EQ(model.field_count, 0);
    ASSERT_EQ(model.field_capacity, 0);
    ASSERT_TRUE(model.fields == NULL);
}

void test_active_model_set_and_get_fields(void) {
    ActiveModel model;
    const char *name_value;
    const char *role_value;

    active_model_init(&model, 1);

    ASSERT_EQ(active_model_set_field(&model, "name", "Alice"), 0);
    ASSERT_EQ(active_model_set_field(&model, "role", "admin"), 0);

    name_value = active_model_get_field(&model, "name");
    role_value = active_model_get_field(&model, "role");

    ASSERT_STR_EQ(name_value, "Alice");
    ASSERT_STR_EQ(role_value, "admin");
}

void test_active_model_update_existing_field(void) {
    ActiveModel model;
    const char *name_value;

    active_model_init(&model, 5);

    ASSERT_EQ(active_model_set_field(&model, "name", "Old"), 0);
    ASSERT_EQ(active_model_set_field(&model, "name", "New"), 0);

    ASSERT_EQ(model.field_count, 1);

    name_value = active_model_get_field(&model, "name");
    ASSERT_STR_EQ(name_value, "New");
}

