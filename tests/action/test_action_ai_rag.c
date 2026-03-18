#include <string.h>

#include "../test_assert.h"
#include "../../core/rag_integration.h"
#include "../../action/action_controller.h"
#include "../../action/action_dispatch.h"

/* Simple global RAG engine used only for this test file. */
static RAGEngine g_rag_engine;

static void rag_controller(ActionRequest *req, ActionResponse *res) {
    char out[256];

    (void)req;

    if (rag_engine_query(&g_rag_engine,
                         "What happened in the database?",
                         out,
                         (int)sizeof(out)) != 0) {
        action_controller_render_text(res, 500, "rag error");
        return;
    }

    /* Keep the response body in a static buffer so its lifetime
     * outlives the controller function. */
    static char body[256];
    snprintf(body, sizeof(body), "%s", out);

    res->status = 200;
    res->body = body;
}

void test_action_ai_rag_endpoint(void) {
    ActionRouter router;
    ActionRequest req;
    ActionResponse res;

    action_router_init(&router);
    rag_engine_init(&g_rag_engine, "gpt-mock");

    /* Store a single memory that should be retrieved for our query. */
    ASSERT_EQ(rag_engine_store(&g_rag_engine,
                               "db_outage",
                               "Database connection failed overnight"),
              0);

    ASSERT_EQ(action_router_add_route(&router,
                                      "GET",
                                      "/rag",
                                      rag_controller),
              0);

    req.method = "GET";
    req.path = "/rag";
    req.body = NULL;

    res.status = 0;
    res.body = NULL;

    /* Full RAG-powered flow: router -> controller -> RAGEngine -> NeuralRuntime. */
    ASSERT_EQ(action_dispatch(&router, &req, &res), 0);
    ASSERT_EQ(res.status, 200);
    ASSERT_TRUE(res.body != NULL);

    /* We can't inspect the exact prompt sent to neural_run, but we can
     * ensure the response exists and that the RAG pipeline executed. */
    ASSERT_TRUE(strstr(res.body, "mock-response") != NULL);
}

