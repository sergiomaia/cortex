#include "../test_assert.h"
#include "../../action/action_request.h"
#include "../../action/action_response.h"

static void parse_simple_get_request(char *raw, ActionRequest *out_req) {
    /* Extremely simple parser for tests:
     * "GET /path HTTP/1.1\r\n\r\n"
     *
     * It mutates the input buffer to insert '\0' terminators so that
     * method and path become standalone C strings. */
    char *space1 = NULL;
    char *space2 = NULL;

    out_req->method = NULL;
    out_req->path = NULL;
    out_req->body = NULL;

    if (!raw) {
        return;
    }

    space1 = raw;
    while (*space1 && *space1 != ' ') {
        ++space1;
    }
    if (*space1 != ' ') {
        return;
    }

    *space1 = '\0';

    space2 = space1 + 1;
    out_req->path = space2;

    while (*space2 && *space2 != ' ') {
        ++space2;
    }
    if (*space2 != ' ') {
        return;
    }

    *space2 = '\0';

    out_req->method = raw;
}

void test_action_request_parse_simple_get(void) {
    char raw[] = "GET /hello HTTP/1.1\r\n\r\n";
    ActionRequest req;

    parse_simple_get_request(raw, &req);

    ASSERT_STR_EQ(req.method, "GET");
    ASSERT_STR_EQ(req.path, "/hello");
}

void test_action_response_set_formats_fields(void) {
    ActionResponse res;

    res.status = 0;
    res.body = NULL;

    action_response_set(&res, 201, "created");

    ASSERT_EQ(res.status, 201);
    ASSERT_STR_EQ(res.body, "created");
}

