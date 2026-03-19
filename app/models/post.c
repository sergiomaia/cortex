/* Auto-generated scaffold model: post */
#include "core/active_record.h"
#include "core/active_model.h"

#define POST_MODEL_NAME "post"

typedef ActiveModel Post;

Post *post_create(ActiveRecordStore *store) {
    return active_record_create(store, POST_MODEL_NAME);
}

int post_set_title(Post *m, const char *title) {
    return active_model_set_field(m, "title", title);
}

