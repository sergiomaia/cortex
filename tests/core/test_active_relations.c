#include <stdlib.h>

#include "../test_assert.h"
#include "../../core/active_relation.h"

/* One-to-many: Incident has many Logs.
 */
void test_active_relations_incident_has_many_logs(void) {
    Incident incident1;
    Incident incident2;
    IncidentLog *log1;
    IncidentLog *log2;
    IncidentLog *log3;
    IncidentLogList logs_for_incident;

    active_relations_init();

    /* Create incidents. */
    incident1 = incident_new(1, "DB outage");
    incident2 = incident_new(2, "API latency");

    /* Create three logs linked via foreign key incident_id (stored as model_id). */
    log1 = incident_log_create(incident1.id, "Initial detection");
    log2 = incident_log_create(incident1.id, "Mitigation in progress");
    log3 = incident_log_create(incident2.id, "Spike observed");

    ASSERT_TRUE(log1 != NULL);
    ASSERT_TRUE(log2 != NULL);
    ASSERT_TRUE(log3 != NULL);

    ASSERT_EQ(log1->model_id, incident1.id);
    ASSERT_EQ(log2->model_id, incident1.id);
    ASSERT_EQ(log3->model_id, incident2.id);

    /* Retrieve via `incident.logs(&incident)` (Rails-style `incident.logs()`). */
    logs_for_incident = incident1.logs(&incident1);
    ASSERT_TRUE(logs_for_incident.items != NULL);
    ASSERT_EQ(logs_for_incident.count, 2);

    ASSERT_TRUE(logs_for_incident.items[0] != NULL);
    ASSERT_TRUE(logs_for_incident.items[1] != NULL);
    ASSERT_EQ(logs_for_incident.items[0]->model_id, incident1.id);
    ASSERT_EQ(logs_for_incident.items[1]->model_id, incident1.id);

    /* Ensure we can fetch logs for the second incident independently. */
    free(logs_for_incident.items);
    logs_for_incident = incident2.logs(&incident2);

    ASSERT_TRUE(logs_for_incident.items != NULL);
    ASSERT_EQ(logs_for_incident.count, 1);
    ASSERT_TRUE(logs_for_incident.items[0] != NULL);
    ASSERT_EQ(logs_for_incident.items[0]->model_id, incident2.id);

    free(logs_for_incident.items);
    active_relations_free();
}

