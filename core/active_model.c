#include <stdlib.h>
#include <string.h>

#include "active_model.h"

#include "cortex_error.h"

static int ensure_fields_capacity(ActiveModel *model) {
    int new_capacity;
    ActiveModelField *new_fields;

    if (!model) {
        CORTEX_SET_ERROR(CORTEX_ERR_INVALID_ARGUMENT, "active:active_model.ensure_fields_capacity",
                         "ActiveModel pointer is NULL");
        return -1;
    }

    if (model->field_capacity == 0) {
        new_capacity = 4;
    } else if (model->field_count >= model->field_capacity) {
        new_capacity = model->field_capacity * 2;
    } else {
        return 0;
    }

    new_fields = (ActiveModelField *)realloc(model->fields, (size_t)new_capacity * sizeof(ActiveModelField));
    if (!new_fields) {
        CORTEX_SET_ERROR(CORTEX_ERR_CORE_OOM, "active:active_model.ensure_fields_capacity",
                         "unable to grow dynamic field backing store");
        return -1;
    }

    model->fields = new_fields;
    model->field_capacity = new_capacity;
    return 0;
}

void active_model_init(ActiveModel *model, int id) {
    if (!model) {
        return;
    }

    model->id = id;
    model->name = NULL;
    model->fields = NULL;
    model->field_count = 0;
    model->field_capacity = 0;
}

int active_model_set_field(ActiveModel *model, const char *key, const char *value) {
    int i;

    if (!model || !key || key[0] == '\0') {
        CORTEX_SET_ERROR(CORTEX_ERR_ACTIVE_VALIDATION, "active:active_model_set_field",
                         "field key is required");
        return -1;
    }

    /* Update existing field if key already present. */
    for (i = 0; i < model->field_count; ++i) {
        if (model->fields[i].key && strcmp(model->fields[i].key, key) == 0) {
            model->fields[i].value = value;
            cortex_clear_error();
            return 0;
        }
    }

    if (ensure_fields_capacity(model) != 0) {
        return -1;
    }

    model->fields[model->field_count].key = key;
    model->fields[model->field_count].value = value;
    model->field_count += 1;

    cortex_clear_error();
    return 0;
}

const char *active_model_get_field(ActiveModel *model, const char *key) {
    int i;

    if (!model || !key) {
        return NULL;
    }

    for (i = 0; i < model->field_count; ++i) {
        if (model->fields[i].key && strcmp(model->fields[i].key, key) == 0) {
            return model->fields[i].value;
        }
    }

    return NULL;
}

