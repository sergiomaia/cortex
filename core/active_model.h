#ifndef ACTIVE_MODEL_H
#define ACTIVE_MODEL_H

typedef struct {
    const char *key;
    const char *value;
} ActiveModelField;

typedef struct {
    int id;
    const char *name;
    ActiveModelField *fields;
    int field_count;
    int field_capacity;
} ActiveModel;

void active_model_init(ActiveModel *model, int id);
int active_model_set_field(ActiveModel *model, const char *key, const char *value);
const char *active_model_get_field(ActiveModel *model, const char *key);

#endif /* ACTIVE_MODEL_H */

 