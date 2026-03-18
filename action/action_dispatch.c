#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "action_dispatch.h"
#include "action_response.h"

int action_dispatch(ActionRouter *router, ActionRequest *req, ActionResponse *res) {
    ActionHandler handler;
    if (!router || !req || !res) {
        return -1;
    }
    handler = action_router_match(router, req->method, req->path);
    if (!handler) {
        action_response_set(res, 404, "Not Found");
        return -1;
    }
    handler(req, res);
    return 0;
}

/* Very small, synchronous HTTP server. Listens on port 3000 and handles
 * one request per accepted connection.
 */
int action_dispatch_serve_http(ActionRouter *router) {
    int server_fd, client_fd;
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);
    char buffer[1024];

    if (!router) {
        return -1;
    }

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        return -1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(3000);

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(server_fd);
        return -1;
    }

    if (listen(server_fd, 8) < 0) {
        close(server_fd);
        return -1;
    }

    for (;;) {
        ssize_t nread;
        char method[8] = {0};
        char path[256] = {0};
        ActionRequest req;
        ActionResponse res;
        char response[1024];
        const char *status_text;

        client_fd = accept(server_fd, (struct sockaddr *)&addr, &addrlen);
        if (client_fd < 0) {
            break;
        }

        nread = read(client_fd, buffer, sizeof(buffer) - 1);
        if (nread <= 0) {
            close(client_fd);
            continue;
        }
        buffer[nread] = '\0';

        /* Parse a very simple "GET /path HTTP/1.1" request line. */
        if (sscanf(buffer, "%7s %255s", method, path) != 2) {
            close(client_fd);
            continue;
        }

        req.method = method;
        req.path = path;
        req.body = NULL;

        res.status = 200;
        res.body = "OK";

        action_dispatch(router, &req, &res);

        if (res.status == 404) {
            status_text = "Not Found";
        } else if (res.status == 200) {
            status_text = "OK";
        } else {
            status_text = "Internal Server Error";
        }

        snprintf(response, sizeof(response),
                 "HTTP/1.1 %d %s\r\n"
                 "Content-Type: text/plain\r\n"
                 "Connection: close\r\n"
                 "\r\n"
                 "%s",
                 res.status, status_text,
                 res.body ? res.body : "");

        write(client_fd, response, strlen(response));
        close(client_fd);
    }

    close(server_fd);
    return 0;
}

