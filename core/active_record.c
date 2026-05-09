#include <stdlib.h>
#include <string.h>

#include "active_record.h"
#include "active_model.h"
#include "cortex_error.h"

void active_record_init(ActiveRecordStore *store) {
    if (!store) {
        return;
    }

    store->items = NULL;
    store->count = 0;
    store->capacity = 0;
    store->next_id = 1;
}

static int ensure_capacity(ActiveRecordStore *store) {
    int new_capacity;
    ActiveModel *new_items;

    if (store->capacity == 0) {
        new_capacity = 4;
    } else if (store->count >= store->capacity) {
        new_capacity = store->capacity * 2;
    } else {
        return 0;
    }

    new_items = (ActiveModel *)realloc(store->items, (size_t)new_capacity * sizeof(ActiveModel));
    if (!new_items) {
        CORTEX_SET_ERROR(CORTEX_ERR_CORE_OOM, "active:active_record.ensure_capacity",
                         "unable to extend in-memory ActiveRecord backing store");
        return -1;
    }

    store->items = new_items;
    store->capacity = new_capacity;
    return 0;
}

ActiveModel *active_record_create(ActiveRecordStore *store, const char *name) {
    ActiveModel *model;
    if (!store || !name) {
        CORTEX_SET_ERROR(CORTEX_ERR_INVALID_ARGUMENT, "active:active_record_create",
                         "store and name pointers are required");
        return NULL;
    }
    if (ensure_capacity(store) != 0) {
        return NULL;
    }

    model = &store->items[store->count];
    active_model_init(model, store->next_id++);
    model->name = name;
    store->count += 1;

    cortex_clear_error();
    return model;
}

int active_record_save(ActiveRecordStore *store, ActiveModel *model) {
    int i;
    if (!store || !model) {
        CORTEX_SET_ERROR(CORTEX_ERR_INVALID_ARGUMENT, "active:active_record_save",
                         "store and model pointers are required");
        return -1;
    }
    for (i = 0; i < store->count; ++i) {
        if (store->items[i].id == model->id) {
            store->items[i] = *model;
            cortex_clear_error();
            return 0;
        }
    }
    CORTEX_SET_ERRORF(CORTEX_ERR_ACTIVE_NOT_FOUND, "active:active_record_save",
                      "cannot persist unknown model id %d", model->id);
    return -1;
}

ActiveModel *active_record_find(ActiveRecordStore *store, int id) {
    int i;
    if (!store || id <= 0) {
        CORTEX_SET_ERROR(CORTEX_ERR_INVALID_ARGUMENT, "active:active_record_find",
                         "invalid store pointer or record id");
        return NULL;
    }
    for (i = 0; i < store->count; ++i) {
        if (store->items[i].id == id) {
            cortex_clear_error();
            return &store->items[i];
        }
    }
    CORTEX_SET_ERRORF(CORTEX_ERR_ACTIVE_NOT_FOUND, "active:active_record_find",
                      "record id %d not found", id);
    return NULL;
}

int active_record_delete(ActiveRecordStore *store, int id) {
    int i;
    if (!store || id <= 0) {
        CORTEX_SET_ERROR(CORTEX_ERR_INVALID_ARGUMENT, "active:active_record_delete",
                         "invalid store pointer or record id");
        return -1;
    }
    for (i = 0; i < store->count; ++i) {
        if (store->items[i].id == id) {
            int last = store->count - 1;
            if (i != last) {
                store->items[i] = store->items[last];
            }
            store->count -= 1;
            cortex_clear_error();
            return 0;
        }
    }
    CORTEX_SET_ERRORF(CORTEX_ERR_ACTIVE_NOT_FOUND, "active:active_record_delete",
                      "record id %d not found", id);
    return -1;
}

void active_record_free(ActiveRecordStore *store) {
    if (!store) {
        return;
    }
    free(store->items);
    store->items = NULL;
    store->count = 0;
    store->capacity = 0;
}
