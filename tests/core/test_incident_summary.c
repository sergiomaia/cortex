#include <string.h>

#include "../test_assert.h"
#include "../../core/incident_summary.h"

void test_incident_summary_builds_prompt_and_returns_response(void) {
    IncidentSummary m;
    char out[512];

    incident_summary_init(&m, "gpt-mock");

    ASSERT_EQ(incident_summary_run(&m,
                                     7,
                                     "DB outage",
                                     "connections failing",
                                     out,
                                     (int)sizeof(out)),
              0);

    /* Validate prompt was built with correct template replacement. */
    ASSERT_STR_EQ(incident_summary_last_prompt(&m),
                   "Incident Summary for incident 7: DB outage. Details: connections failing.");

    /* Ensure response is returned. */
    ASSERT_TRUE(out[0] != '\0');

    /* Validate output contains expected keywords. */
    ASSERT_TRUE(strstr(out, "mock-response") != NULL);
    ASSERT_TRUE(strstr(out, "summary") != NULL);
    ASSERT_TRUE(strstr(out, "incident 7") != NULL);
    ASSERT_TRUE(strstr(out, "DB outage") != NULL);
}

