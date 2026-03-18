#include <stdlib.h>

#include "active_relation.h"

void incident_log_store_init(IncidentLogStore *store) {
    if (!store) {
        return;
    }
    store->items = NULL;
    store->count = 0;
    store->capacity = 0;
    store->next_id = 1;
}

static int incident_log_store_ensure_capacity(IncidentLogStore *store) {
    int new_capacity;
    IncidentLog *new_items;

    if (store->capacity == 0) {
        new_capacity = 4;
    } else if (store->count >= store->capacity) {
        new_capacity = store->capacity * 2;
    } else {
        return 0;
    }

    new_items = (IncidentLog *)realloc(store->items,
                                       (size_t)new_capacity * sizeof(IncidentLog));
    if (!new_items) {
        return -1;
    }

    store->items = new_items;
    store->capacity = new_capacity;
    return 0;
}

IncidentLog *incident_log_store_add(IncidentLogStore *store,
                                    int incident_id,
                                    const char *message) {
    IncidentLog *log;

    if (!store || !message) {
        return NULL;
    }
    if (incident_log_store_ensure_capacity(store) != 0) {
        return NULL;
    }

    log = &store->items[store->count];
    log->id = store->next_id++;
    log->incident_id = incident_id;
    log->message = message;
    store->count += 1;

    return log;
}

void incident_log_store_free(IncidentLogStore *store) {
    if (!store) {
        return;
    }
    free(store->items);
    store->items = NULL;
    store->count = 0;
    store->capacity = 0;
    store->next_id = 0;
}

IncidentLog **incident_logs(const Incident *incident,
                            IncidentLogStore *store,
                            int *out_count) {
    IncidentLog **results;
    int i;
    int matched = 0;

    if (out_count) {
        *out_count = 0;
    }

    if (!incident || !store || store->count <= 0) {
        return NULL;
    }

    results = (IncidentLog **)malloc((size_t)store->count * sizeof(IncidentLog *));
    if (!results) {
        return NULL;
    }

    for (i = 0; i < store->count; ++i) {
        IncidentLog *log = &store->items[i];
        if (log->incident_id == incident->id) {
            results[matched++] = log;
        }
    }

    if (matched == 0) {
        free(results);
        return NULL;
    }

    if (out_count) {
        *out_count = matched;
    }

    return results;
}

