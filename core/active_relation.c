#include <stdlib.h>

#include "active_relation.h"

static IncidentLogStore g_log_store;
static int g_initialized = 0;

static void incident_log_store_init(IncidentLogStore *store) {
    if (!store) {
        return;
    }
    store->items = NULL;
    store->count = 0;
    store->capacity = 0;
    store->next_id = 1;
}

static void incident_log_store_free(IncidentLogStore *store) {
    if (!store) {
        return;
    }
    free(store->items);
    store->items = NULL;
    store->count = 0;
    store->capacity = 0;
    store->next_id = 0;
}

static int incident_log_store_ensure_capacity(IncidentLogStore *store) {
    int new_capacity;
    IncidentLog *new_items;

    if (!store) {
        return -1;
    }

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

void active_relations_init(void) {
    if (g_initialized) {
        return;
    }
    incident_log_store_init(&g_log_store);
    g_initialized = 1;
}

void active_relations_free(void) {
    if (!g_initialized) {
        return;
    }
    incident_log_store_free(&g_log_store);
    g_initialized = 0;
}

static IncidentLogList incident_logs_for_incident(const struct Incident *self);

Incident incident_new(int id, const char *title) {
    Incident inc;

    inc.id = id;
    inc.title = title;
    inc.logs = NULL; /* set below */

    /* "incident.logs()" style: call the function pointer. */
    inc.logs = incident_logs_for_incident;

    return inc;
}

static IncidentLogList incident_logs_for_incident(const struct Incident *self) {
    if (!self) {
        IncidentLogList empty = { .items = NULL, .count = 0 };
        return empty;
    }
    return incident_logs(self->id);
}

IncidentLog *incident_log_create(int incident_id, const char *message) {
    IncidentLog *log;

    if (!g_initialized || !message) {
        return NULL;
    }
    if (incident_log_store_ensure_capacity(&g_log_store) != 0) {
        return NULL;
    }

    log = &g_log_store.items[g_log_store.count];
    log->id = g_log_store.next_id++;

    /* foreign key convention: Logs `model_id` points to Incident.id */
    log->model_id = incident_id;
    log->message = message;

    g_log_store.count += 1;
    return log;
}

IncidentLogList incident_logs(int incident_id) {
    IncidentLogList list;
    int i;
    int matched;

    list.items = NULL;
    list.count = 0;

    if (!g_initialized || g_log_store.count <= 0) {
        return list;
    }

    matched = 0;
    for (i = 0; i < g_log_store.count; ++i) {
        if (g_log_store.items[i].model_id == incident_id) {
            matched += 1;
        }
    }

    if (matched <= 0) {
        return list;
    }

    list.items = (IncidentLog **)malloc((size_t)matched * sizeof(IncidentLog *));
    if (!list.items) {
        return list;
    }

    list.count = matched;

    matched = 0;
    for (i = 0; i < g_log_store.count; ++i) {
        IncidentLog *log = &g_log_store.items[i];
        if (log->model_id == incident_id) {
            list.items[matched++] = log;
        }
    }

    return list;
}

