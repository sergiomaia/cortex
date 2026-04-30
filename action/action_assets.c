#include "action_assets.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int read_manifest_entry(char *out, size_t out_size) {
    FILE *f = fopen("public/assets/manifest.json", "rb");
    long len;
    char *buf;
    char *key;
    char *colon;
    char *q1;
    char *q2;
    int ok = -1;

    if (!f || !out || out_size == 0) {
        if (f) fclose(f);
        return -1;
    }

    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return -1; }
    len = ftell(f);
    if (len <= 0) { fclose(f); return -1; }
    if (fseek(f, 0, SEEK_SET) != 0) { fclose(f); return -1; }

    buf = (char *)malloc((size_t)len + 1);
    if (!buf) { fclose(f); return -1; }
    if (fread(buf, 1, (size_t)len, f) != (size_t)len) { free(buf); fclose(f); return -1; }
    buf[len] = '\0';
    fclose(f);

    key = strstr(buf, "\"application.js\"");
    if (!key) { free(buf); return -1; }
    colon = strchr(key, ':');
    if (!colon) { free(buf); return -1; }
    q1 = strchr(colon, '"');
    if (!q1) { free(buf); return -1; }
    ++q1;
    q2 = strchr(q1, '"');
    if (!q2) { free(buf); return -1; }

    {
        size_t n = (size_t)(q2 - q1);
        if (n >= out_size) n = out_size - 1;
        memcpy(out, q1, n);
        out[n] = '\0';
        ok = 0;
    }

    free(buf);
    return ok;
}

const char *action_assets_javascript_path(void) {
    static char logical[256];
    static int initialized = 0;
    if (!initialized) {
        char compiled[128];
        if (read_manifest_entry(compiled, sizeof(compiled)) == 0) {
            snprintf(logical, sizeof(logical), "/assets/%s", compiled);
        } else {
            snprintf(logical, sizeof(logical), "/assets/application.js");
        }
        initialized = 1;
    }
    return logical;
}

char *action_assets_inject_javascript(const char *html) {
    const char *script_path;
    const char *needle = "</body>";
    const char *pos;
    const char *script_fmt = "  <script type=\"module\" src=\"%s\"></script>\n";
    int script_len;
    size_t html_len;
    size_t prefix_len;
    char *out;

    if (!html) return NULL;
    script_path = action_assets_javascript_path();
    script_len = snprintf(NULL, 0, script_fmt, script_path);
    if (script_len <= 0) return NULL;

    html_len = strlen(html);
    pos = strstr(html, needle);
    if (!pos) {
        out = (char *)malloc(html_len + (size_t)script_len + 1);
        if (!out) return NULL;
        memcpy(out, html, html_len);
        snprintf(out + html_len, (size_t)script_len + 1, script_fmt, script_path);
        return out;
    }

    prefix_len = (size_t)(pos - html);
    out = (char *)malloc(html_len + (size_t)script_len + 1);
    if (!out) return NULL;
    memcpy(out, html, prefix_len);
    snprintf(out + prefix_len, (size_t)script_len + 1, script_fmt, script_path);
    strcpy(out + prefix_len + (size_t)script_len, pos);
    return out;
}

static int asset_rel_path_safe(const char *rel) {
    if (!rel || rel[0] == '\0') {
        return -1;
    }
    if (rel[0] == '/') {
        return -1;
    }
    if (strstr(rel, "..") != NULL) {
        return -1;
    }
    return 0;
}

static const char *guess_content_type(const char *request_path) {
    if (strstr(request_path, ".css") != NULL) {
        return "text/css; charset=utf-8";
    }
    if (strstr(request_path, ".js") != NULL) {
        return "application/javascript";
    }
    if (strstr(request_path, ".json") != NULL) {
        return "application/json";
    }
    if (strstr(request_path, ".png") != NULL) {
        return "image/png";
    }
    if (strstr(request_path, ".jpg") != NULL || strstr(request_path, ".jpeg") != NULL) {
        return "image/jpeg";
    }
    if (strstr(request_path, ".gif") != NULL) {
        return "image/gif";
    }
    if (strstr(request_path, ".svg") != NULL) {
        return "image/svg+xml";
    }
    if (strstr(request_path, ".webp") != NULL) {
        return "image/webp";
    }
    if (strstr(request_path, ".ico") != NULL) {
        return "image/x-icon";
    }
    if (strstr(request_path, ".woff2") != NULL) {
        return "font/woff2";
    }
    if (strstr(request_path, ".woff") != NULL) {
        return "font/woff";
    }
    return "text/plain";
}

int action_assets_serve_static_path(const char *request_path, int client_fd) {
    char fs_path[512];
    FILE *f;
    long len;
    char *body;
    const char *ctype;
    char header[512];
    int header_len;

    if (!request_path || strncmp(request_path, "/assets/", 8) != 0 || client_fd < 0) {
        return -1;
    }
    if (strstr(request_path, "..") != NULL) {
        return -1;
    }
    if (snprintf(fs_path, sizeof(fs_path), "public%s", request_path) < 0) {
        return -1;
    }
    f = fopen(fs_path, "rb");
    if (!f) {
        const char *rel = request_path + 8;
        if (asset_rel_path_safe(rel) != 0) {
            return -1;
        }
        if (snprintf(fs_path, sizeof(fs_path), "app/assets/%s", rel) >= (int)sizeof(fs_path)) {
            return -1;
        }
        f = fopen(fs_path, "rb");
        if (!f) {
            return -1;
        }
    }
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return -1; }
    len = ftell(f);
    if (len < 0) { fclose(f); return -1; }
    if (fseek(f, 0, SEEK_SET) != 0) { fclose(f); return -1; }
    body = (char *)malloc((size_t)len);
    if (!body) { fclose(f); return -1; }
    if (len > 0 && fread(body, 1, (size_t)len, f) != (size_t)len) {
        free(body);
        fclose(f);
        return -1;
    }
    fclose(f);

    ctype = guess_content_type(request_path);

    header_len = snprintf(header, sizeof(header),
                          "HTTP/1.1 200 OK\r\n"
                          "Content-Type: %s\r\n"
                          "Connection: close\r\n"
                          "\r\n",
                          ctype);
    if (header_len > 0) write(client_fd, header, (size_t)header_len);
    if (len > 0) write(client_fd, body, (size_t)len);
    free(body);
    return 0;
}
