#ifndef ACTIVE_RECORD_H
#define ACTIVE_RECORD_H

#include "active_model.h"

typedef struct {
    ActiveModel *items;
    int count;
    int capacity;
    int next_id;
} ActiveRecordStore;

void active_record_init(ActiveRecordStore *store);
ActiveModel *active_record_create(ActiveRecordStore *store, const char *name);
int active_record_save(ActiveRecordStore *store, ActiveModel *model);
ActiveModel *active_record_find(ActiveRecordStore *store, int id);
int active_record_delete(ActiveRecordStore *store, int id);
void active_record_free(ActiveRecordStore *store);

#endif /* ACTIVE_RECORD_H */

