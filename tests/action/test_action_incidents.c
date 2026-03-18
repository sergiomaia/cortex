#include <stdio.h>

#include "../test_assert.h"
#include "../../core/active_record.h"
#include "../../action/action_controller.h"
#include "../../action/action_dispatch.h"

static ActiveRecordStore incidents_store;
static char incidents_json_buffer[256];

static void incidents_controller(ActionRequest *req, ActionResponse *res) {
    int id = 0;
    ActiveModel *incident;

    /* Extract ":id" from the path, e.g. "/incidents/42". */
    if (sscanf(req->path, "/incidents/%d", &id) != 1) {
        action_controller_render_json(res, 400, "{\"error\":\"bad_request\"}");
        return;
    }

    incident = active_record_find(&incidents_store, id);
    if (!incident) {
        action_controller_render_json(res, 404, "{\"error\":\"not_found\"}");
        return;
    }

    /* Build a simple JSON payload and keep it in a static buffer so the
     * response body pointer remains valid for the duration of the test. */
    snprintf(incidents_json_buffer, sizeof(incidents_json_buffer),
             "{\"id\":%d,\"name\":\"%s\"}", incident->id, incident->name);

    action_controller_render_json(res, 200, incidents_json_buffer);
}

void test_action_incidents_show_integration(void) {
    ActionRouter router;
    ActionRequest req;
    ActionResponse res;
    ActiveModel *created;

    active_record_init(&incidents_store);

    /* Mock model data in an in-memory store. */
    created = active_record_create(&incidents_store, "Network outage");
    ASSERT_TRUE(created != NULL);

    action_router_init(&router);
    ASSERT_EQ(action_router_add_route(&router, "GET", "/incidents/:id", incidents_controller), 0);

    req.method = "GET";
    req.path = "/incidents/1";
    req.body = NULL;

    res.status = 0;
    res.body = NULL;

    /* Full integration flow: router -> controller -> response. */
    ASSERT_EQ(action_dispatch(&router, &req, &res), 0);
    ASSERT_EQ(res.status, 200);
    ASSERT_TRUE(res.body != NULL);
    ASSERT_TRUE(strstr(res.body, "\"id\":1") != NULL);
    ASSERT_TRUE(strstr(res.body, "Network outage") != NULL);

    active_record_free(&incidents_store);
}

