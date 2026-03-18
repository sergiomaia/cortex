#include <stdlib.h>
#include <string.h>

#include "active_query.h"

void active_query_init(ActiveQuery *query, ActiveRecordStore *store) {
    if (!query) {
        return;
    }
    query->store = store;
    query->where_field = NULL;
    query->where_value = NULL;
    query->has_where = 0;
    query->limit = 0;
}

ActiveQuery *active_query_where(ActiveQuery *query, const char *field, const char *value) {
    if (!query || !field || !value) {
        return query;
    }
    query->where_field = field;
    query->where_value = value;
    query->has_where = 1;
    return query;
}

ActiveQuery *active_query_limit(ActiveQuery *query, int limit) {
    if (!query) {
        return query;
    }
    query->limit = limit;
    return query;
}

static int matches_where(ActiveQuery *query, ActiveModel *model) {
    if (!query || !model) {
        return 0;
    }
    if (!query->has_where || !query->where_field) {
        return 1;
    }

    /* Basic equality-based filtering on known fields. */
    if (strcmp(query->where_field, "name") == 0) {
        if (!model->name || !query->where_value) {
            return 0;
        }
        return strcmp(model->name, query->where_value) == 0;
    }

    if (strcmp(query->where_field, "id") == 0) {
        if (!query->where_value) {
            return 0;
        }
        return model->id == atoi(query->where_value);
    }

    /* Unknown field: treat as non-match. */
    return 0;
}

ActiveModel **active_query_execute(ActiveQuery *query, int *out_count) {
    ActiveModel **results;
    int i;
    int matched = 0;
    int effective_limit;
    ActiveRecordStore *store;

    if (out_count) {
        *out_count = 0;
    }

    if (!query || !query->store) {
        return NULL;
    }

    store = query->store;
    if (store->count <= 0) {
        return NULL;
    }

    results = (ActiveModel **)malloc(sizeof(ActiveModel *) * store->count);
    if (!results) {
        return NULL;
    }

    effective_limit = store->count;
    if (query->limit > 0 && query->limit < effective_limit) {
        effective_limit = query->limit;
    }

    for (i = 0; i < store->count && matched < effective_limit; ++i) {
        ActiveModel *model = &store->items[i];
        if (matches_where(query, model)) {
            results[matched++] = model;
        }
    }

    if (out_count) {
        *out_count = matched;
    }

    if (matched == 0) {
        free(results);
        return NULL;
    }

    return results;
}

