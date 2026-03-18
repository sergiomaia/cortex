#include <string.h>

#include "../test_assert.h"
#include "../../core/incident_summary.h"
#include "../../action/action_controller.h"
#include "../../action/action_dispatch.h"

static char ai_incident_body[512];

static void ai_incident_summary_controller(ActionRequest *req, ActionResponse *res) {
    int id = 0;
    IncidentSummary summary;
    char summary_out[256];

    /* Parse "/incidents/:id" from the request path. */
    if (sscanf(req->path, "/incidents/%d", &id) != 1) {
        action_controller_render_text(res, 400, "bad request");
        return;
    }

    incident_summary_init(&summary, "gpt-mock");

    if (incident_summary_run(&summary,
                             id,
                             "AI incident",
                             "details for testing AI summary",
                             summary_out,
                             (int)sizeof(summary_out)) != 0) {
        action_controller_render_text(res, 500, "summary error");
        return;
    }

    /* Keep the response body in a static buffer so its lifetime
     * outlives the controller function. */
    snprintf(ai_incident_body, sizeof(ai_incident_body), "%s", summary_out);

    res->status = 200;
    res->body = ai_incident_body;
}

void test_action_ai_incident_summary_endpoint(void) {
    ActionRouter router;
    ActionRequest req;
    ActionResponse res;

    action_router_init(&router);
    ASSERT_EQ(action_router_add_route(&router, "GET", "/incidents/:id", ai_incident_summary_controller), 0);

    req.method = "GET";
    req.path = "/incidents/3";
    req.body = NULL;

    res.status = 0;
    res.body = NULL;

    /* Full AI-powered flow: router -> controller -> IncidentSummary -> response. */
    ASSERT_EQ(action_dispatch(&router, &req, &res), 0);
    ASSERT_EQ(res.status, 200);
    ASSERT_TRUE(res.body != NULL);

    /* Ensure response includes a summary and validates AI output format. */
    ASSERT_TRUE(strstr(res.body, "mock-response") != NULL);
    ASSERT_TRUE(strstr(res.body, "summary for incident 3") != NULL);
    ASSERT_TRUE(strstr(res.body, "AI incident") != NULL);
}

