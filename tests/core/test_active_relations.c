#include <stdlib.h>

#include "../test_assert.h"
#include "../../core/active_relation.h"

/* One-to-many: Incident has many Logs.
 *
 * We keep this intentionally simple and separate from ActiveRecord:
 *  - Incident has an id (like a primary key).
 *  - IncidentLog has an incident_id foreign key plus a message.
 *  - IncidentLogStore stores all logs in memory.
 *
 * The helper `incident_logs(incident, store, &count)` is the C
 * equivalent of `incident.logs()`.
 */
void test_active_relations_incident_has_many_logs(void) {
    Incident incident1 = { .id = 1, .title = "DB outage" };
    Incident incident2 = { .id = 2, .title = "API latency" };
    IncidentLogStore log_store;
    IncidentLog *log1;
    IncidentLog *log2;
    IncidentLog *log3;
    IncidentLog **logs_for_incident;
    int count = 0;

    incident_log_store_init(&log_store);

    /* Create three logs linked via foreign key incident_id. */
    log1 = incident_log_store_add(&log_store, incident1.id, "Initial detection");
    log2 = incident_log_store_add(&log_store, incident1.id, "Mitigation in progress");
    log3 = incident_log_store_add(&log_store, incident2.id, "Spike observed");

    ASSERT_TRUE(log1 != NULL);
    ASSERT_TRUE(log2 != NULL);
    ASSERT_TRUE(log3 != NULL);

    ASSERT_EQ(log1->incident_id, incident1.id);
    ASSERT_EQ(log2->incident_id, incident1.id);
    ASSERT_EQ(log3->incident_id, incident2.id);

    /* This is our "incident.logs()" helper in C form. */
    logs_for_incident = incident_logs(&incident1, &log_store, &count);

    ASSERT_TRUE(logs_for_incident != NULL);
    ASSERT_EQ(count, 2);

    ASSERT_TRUE(logs_for_incident[0] != NULL);
    ASSERT_TRUE(logs_for_incident[1] != NULL);

    ASSERT_EQ(logs_for_incident[0]->incident_id, incident1.id);
    ASSERT_EQ(logs_for_incident[1]->incident_id, incident1.id);

    /* Ensure we can fetch logs for the second incident independently. */
    free(logs_for_incident);
    logs_for_incident = incident_logs(&incident2, &log_store, &count);

    ASSERT_TRUE(logs_for_incident != NULL);
    ASSERT_EQ(count, 1);
    ASSERT_TRUE(logs_for_incident[0] != NULL);
    ASSERT_EQ(logs_for_incident[0]->incident_id, incident2.id);

    free(logs_for_incident);
    incident_log_store_free(&log_store);
}

