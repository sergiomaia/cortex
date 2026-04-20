#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "action_dispatch.h"
#include "action_response.h"
#include "action_router.h"
#include "action_middleware.h"
#include "action_assets.h"
#include "action_request_form.h"
#include "../core/core_config.h"

#define HTTP_REQ_MAX 65536

static int line_has_prefix_ci(const char *line, size_t line_len, const char *pref) {
    size_t pl = strlen(pref);
    size_t i;
    if (line_len < pl) {
        return 0;
    }
    for (i = 0; i < pl; i++) {
        char c = line[i];
        char p = pref[i];
        if (c >= 'A' && c <= 'Z') {
            c = (char)(c - 'A' + 'a');
        }
        if (c != p) {
            return 0;
        }
    }
    return 1;
}

static int parse_content_length_scan(const char *start, size_t len) {
    size_t i = 0;

    while (i < len) {
        size_t j = i;
        while (j < len && start[j] != '\r') {
            j++;
        }
        if (j > i && line_has_prefix_ci(start + i, j - i, "content-length:")) {
            const char *colon = (const char *)memchr(start + i, ':', j - i);
            if (colon) {
                const char *v = colon + 1;
                while (v < start + j && (*v == ' ' || *v == '\t')) {
                    v++;
                }
                return atoi(v);
            }
        }
        if (j + 1 < len && start[j] == '\r' && start[j + 1] == '\n') {
            i = j + 2;
        } else {
            break;
        }
    }
    return -1;
}

/* Read until headers complete; then read body per Content-Length. Returns 0 on success. */
static int http_read_full_request(int fd, char *buf, size_t cap, char **body_out, size_t *body_len_out) {
    size_t total = 0;
    ssize_t n;
    char *hend;
    size_t header_end_idx;
    int cl;
    size_t need;

    *body_out = NULL;
    *body_len_out = 0;

    if (cap < 16) {
        return -1;
    }

    while (total < cap - 1) {
        n = read(fd, buf + total, cap - 1 - total);
        if (n < 0) {
            return -1;
        }
        if (n == 0) {
            if (total == 0) {
                return -1;
            }
            break;
        }
        total += (size_t)n;
        buf[total] = '\0';
        hend = strstr(buf, "\r\n\r\n");
        if (hend) {
            break;
        }
        if (total > 8192) {
            return -1;
        }
    }

    hend = strstr(buf, "\r\n\r\n");
    if (!hend) {
        return -1;
    }

    header_end_idx = (size_t)(hend - buf);
    cl = parse_content_length_scan(buf, header_end_idx);
    if (cl < 0) {
        cl = 0;
    }
    if ((size_t)cl > cap - 1 || header_end_idx + 4 > cap - 1) {
        return -1;
    }
    need = header_end_idx + 4 + (size_t)cl;
    while (total < need && total < cap - 1) {
        n = read(fd, buf + total, cap - 1 - total);
        if (n <= 0) {
            return -1;
        }
        total += (size_t)n;
    }
    if (total < need) {
        return -1;
    }
    buf[need] = '\0';
    *body_out = buf + header_end_idx + 4;
    *body_len_out = (size_t)cl;
    return 0;
}

static const char *http_status_text(int status) {
    switch (status) {
        case 200:
            return "OK";
        case 201:
            return "Created";
        case 301:
            return "Moved Permanently";
        case 302:
            return "Found";
        case 303:
            return "See Other";
        case 400:
            return "Bad Request";
        case 404:
            return "Not Found";
        case 500:
            return "Internal Server Error";
        default:
            return "OK";
    }
}

int action_dispatch(ActionRouter *router, ActionRequest *req, ActionResponse *res) {
    ActionHandler handler;
    const char *effective_method;
    char method_override[16];
    int i;
    if (!router || !req || !res) {
        return -1;
    }
    effective_method = req->method;
    if (req->method && strcmp(req->method, "POST") == 0 &&
        req->body && req->body[0] &&
        action_request_form_get(req->body, "_method", method_override, sizeof(method_override)) >= 0) {
        for (i = 0; method_override[i]; ++i) {
            method_override[i] = (char)toupper((unsigned char)method_override[i]);
        }
        if (strcmp(method_override, "PUT") == 0 ||
            strcmp(method_override, "PATCH") == 0 ||
            strcmp(method_override, "DELETE") == 0) {
            effective_method = method_override;
        }
    }
    req->method = effective_method;
    handler = action_router_match(router, effective_method, req->path);
    if (!handler) {
        action_response_set(res, 404, "Not Found");
        return -1;
    }
    action_middleware_dispatch(handler, req, res);
    return 0;
}

/* Very small, synchronous HTTP server. Listens on CORE_PORT (default 3000). */
int action_dispatch_serve_http(ActionRouter *router) {
    int server_fd, client_fd;
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);
    char *reqbuf = NULL;
    CoreConfig cfg;
    int port;

    if (!router) {
        return -1;
    }

    cfg = core_config_load();
    port = cfg.port;
    if (port <= 0 || port > 65535) {
        port = 3000;
    }

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("action_dispatch_serve_http: socket failed");
        return -1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons((unsigned short)port);

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("action_dispatch_serve_http: bind failed");
        close(server_fd);
        return -1;
    }

    if (listen(server_fd, 8) < 0) {
        perror("action_dispatch_serve_http: listen failed");
        close(server_fd);
        return -1;
    }

    reqbuf = (char *)malloc(HTTP_REQ_MAX);
    if (!reqbuf) {
        close(server_fd);
        return -1;
    }

    for (;;) {
        char method[8] = {0};
        char path[256] = {0};
        char *req_line_end;
        ActionRequest req;
        ActionResponse res;
        const char *status_text;
        char *body_ptr;
        size_t body_len;

        client_fd = accept(server_fd, (struct sockaddr *)&addr, &addrlen);
        if (client_fd < 0) {
            break;
        }

        if (http_read_full_request(client_fd, reqbuf, HTTP_REQ_MAX, &body_ptr, &body_len) != 0) {
            close(client_fd);
            continue;
        }

        req_line_end = strstr(reqbuf, "\r\n");
        if (!req_line_end) {
            close(client_fd);
            continue;
        }
        *req_line_end = '\0';
        if (sscanf(reqbuf, "%7s %255s", method, path) != 2) {
            *req_line_end = '\r';
            close(client_fd);
            continue;
        }
        *req_line_end = '\r';

        req.method = method;
        req.path = path;
        req.body = body_len > 0 ? body_ptr : "";

        if (action_assets_serve_static_path(req.path, client_fd) == 0) {
            close(client_fd);
            continue;
        }

        res.status = 200;
        res.body = "OK";
        res.content_type = "text/plain";
        res.location = NULL;

        action_dispatch(router, &req, &res);

        status_text = http_status_text(res.status);

        if (!res.content_type) {
            res.content_type = "text/plain";
        }

        {
            const char *body = res.body ? res.body : "";
            size_t body_len_out = strlen(body);
            char header[768];
            int header_len;

            if (res.location && res.location[0]) {
                header_len = snprintf(header, sizeof(header),
                                      "HTTP/1.1 %d %s\r\n"
                                      "Location: %s\r\n"
                                      "Content-Type: %s\r\n"
                                      "Content-Length: %zu\r\n"
                                      "Connection: close\r\n"
                                      "\r\n",
                                      res.status, status_text, res.location, res.content_type, body_len_out);
            } else {
                header_len = snprintf(header, sizeof(header),
                                      "HTTP/1.1 %d %s\r\n"
                                      "Content-Type: %s\r\n"
                                      "Content-Length: %zu\r\n"
                                      "Connection: close\r\n"
                                      "\r\n",
                                      res.status, status_text, res.content_type, body_len_out);
            }
            if (header_len > 0 && header_len < (int)sizeof(header)) {
                write(client_fd, header, (size_t)header_len);
            }
            if (body_len_out > 0) {
                write(client_fd, body, body_len_out);
            }
        }
        close(client_fd);
    }

    free(reqbuf);
    close(server_fd);
    return 0;
}
