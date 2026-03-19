#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "action_view.h"
#include "action_response.h"

void action_view_render(ActionView *view, const char *data, char *buffer, int buffer_size) {
    (void)view;
    (void)data;
    if (!buffer || buffer_size <= 0) {
        return;
    }
    buffer[0] = '\0';
}

void render_view(ActionResponse *res, const char *template_name) {
    char path[512];
    FILE *f;
    char *buf;
    long len;

    if (!res || !template_name) {
        return;
    }

    if (snprintf(path, sizeof(path), "app/views/%s.html", template_name) < 0) {
        action_response_set(res, 500, "Template path error");
        action_response_set_content_type(res, "text/plain");
        return;
    }

    f = fopen(path, "rb");
    if (!f) {
        action_response_set(res, 500, "Template not found");
        action_response_set_content_type(res, "text/plain");
        return;
    }

    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        action_response_set(res, 500, "Template read error");
        action_response_set_content_type(res, "text/plain");
        return;
    }

    len = ftell(f);
    if (len < 0) {
        fclose(f);
        action_response_set(res, 500, "Template read error");
        action_response_set_content_type(res, "text/plain");
        return;
    }
    if (fseek(f, 0, SEEK_SET) != 0) {
        fclose(f);
        action_response_set(res, 500, "Template read error");
        action_response_set_content_type(res, "text/plain");
        return;
    }

    buf = (char *)malloc((size_t)len + 1);
    if (!buf) {
        fclose(f);
        action_response_set(res, 500, "Template allocation error");
        action_response_set_content_type(res, "text/plain");
        return;
    }

    if (fread(buf, 1, (size_t)len, f) != (size_t)len) {
        free(buf);
        fclose(f);
        action_response_set(res, 500, "Template read error");
        action_response_set_content_type(res, "text/plain");
        return;
    }
    buf[len] = '\0';
    fclose(f);

    action_response_set(res, 200, buf);
    action_response_set_content_type(res, "text/html");
}

