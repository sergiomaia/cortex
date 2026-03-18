#ifndef ACTIVE_RELATION_H
#define ACTIVE_RELATION_H

/* Extremely small "relations" layer for an Incident -> Logs one-to-many
 * relationship, implemented without touching the existing ActiveRecord
 * data structures.
 *
 * We model:
 *   - Incident: lightweight record with an id.
 *   - IncidentLog: child record with a foreign key `incident_id`.
 *   - IncidentLogStore: in-memory collection of logs.
 */

typedef struct {
    int id;
    const char *title;
} Incident;

typedef struct {
    int id;
    int incident_id; /* foreign key to Incident.id */
    const char *message;
} IncidentLog;

typedef struct {
    IncidentLog *items;
    int count;
    int capacity;
    int next_id;
} IncidentLogStore;

void incident_log_store_init(IncidentLogStore *store);
IncidentLog *incident_log_store_add(IncidentLogStore *store,
                                    int incident_id,
                                    const char *message);
void incident_log_store_free(IncidentLogStore *store);

/* Helper that mimics `incident.logs()`:
 *
 *   IncidentLog **incident_logs(const Incident *incident,
 *                               IncidentLogStore *store,
 *                               int *out_count)
 *
 * Returns a newly allocated array of IncidentLog* for all logs whose
 * incident_id matches incident->id. Caller owns the returned array
 * (but not the individual logs) and must free() it.
 */
IncidentLog **incident_logs(const Incident *incident,
                            IncidentLogStore *store,
                            int *out_count);

#endif /* ACTIVE_RELATION_H */

