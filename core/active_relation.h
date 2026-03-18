#ifndef ACTIVE_RELATION_H
#define ACTIVE_RELATION_H

/* Extremely small "relations" layer for an Incident -> Logs one-to-many
 * relationship.
 *
 * Requirements this module satisfies:
 *   - One-to-many: Incident has many Logs
 *   - Foreign key convention: Logs store uses `model_id`
 *     (here, Logs' `model_id` points to Incident.id)
 *   - Helper function: `incident_logs(incident_id)`
 *   - Retrieval style: `incident.logs(&incident)`
 */

struct Incident; /* forward */

typedef struct IncidentLog {
    int id;
    int model_id; /* foreign key to Incident.id */
    const char *message;
} IncidentLog;

typedef struct {
    IncidentLog **items;
    int count;
} IncidentLogList;

typedef struct Incident {
    int id;
    const char *title;
    /* "incident.logs()" style retrieval: call as `incident.logs(&incident)` */
    IncidentLogList (*logs)(const struct Incident *self);
} Incident;

/* Global in-memory log store owned by this module. */
typedef struct {
    IncidentLog *items;
    int count;
    int capacity;
    int next_id;
} IncidentLogStore;

void active_relations_init(void);
void active_relations_free(void);

Incident incident_new(int id, const char *title);
IncidentLog *incident_log_create(int incident_id, const char *message);

/* Helper that mimics `incident.logs()`:
 * Returns a newly allocated `IncidentLogList.items` array.
 * Caller owns the returned array (but not the individual logs).
 */
IncidentLogList incident_logs(int incident_id);

#endif /* ACTIVE_RELATION_H */

