#include <stdlib.h>
#include <string.h>

#include "active_record.h"
#include "active_model.h"

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

    new_items = (ActiveModel *)realloc(store->items, new_capacity * sizeof(ActiveModel));
    if (!new_items) {
        return -1;
    }

    store->items = new_items;
    store->capacity = new_capacity;
    return 0;
}

ActiveModel *active_record_create(ActiveRecordStore *store, const char *name) {
    ActiveModel *model;
    if (!store || !name) {
        return NULL;
    }
    if (ensure_capacity(store) != 0) {
        return NULL;
    }

    model = &store->items[store->count];
    active_model_init(model, store->next_id++);
    model->name = name;
    store->count += 1;

    return model;
}

int active_record_save(ActiveRecordStore *store, ActiveModel *model) {
    int i;
    if (!store || !model) {
        return -1;
    }
    for (i = 0; i < store->count; ++i) {
        if (store->items[i].id == model->id) {
            store->items[i] = *model;
            return 0;
        }
    }
    return -1;
}

ActiveModel *active_record_find(ActiveRecordStore *store, int id) {
    int i;
    if (!store || id <= 0) {
        return NULL;
    }
    for (i = 0; i < store->count; ++i) {
        if (store->items[i].id == id) {
            return &store->items[i];
        }
    }
    return NULL;
}

int active_record_delete(ActiveRecordStore *store, int id) {
    int i;
    if (!store || id <= 0) {
        return -1;
    }
    for (i = 0; i < store->count; ++i) {
        if (store->items[i].id == id) {
            int last = store->count - 1;
            if (i != last) {
                store->items[i] = store->items[last];
            }
            store->count -= 1;
            return 0;
        }
    }
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

