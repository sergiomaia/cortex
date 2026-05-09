#include "action_error.h"

#include <stdio.h>
#include <string.h>

enum { action_error_tls_cap = 4096 };

static _Thread_local char action_error_tls[action_error_tls_cap];

static void action_error_json_escape(const char *in, char *out, size_t cap)
{
    size_t pos = 0;

    if (!out || cap == 0) {
        return;
    }
    out[0] = '\0';
    if (!in) {
        return;
    }

    while (*in != '\0' && pos + 1 < cap) {
        unsigned char u = (unsigned char)*in;

        if (u == (unsigned char)'\\') {
            if (pos + 3 >= cap) {
                break;
            }
            out[pos++] = '\\';
            out[pos++] = '\\';
        } else if (u == (unsigned char)'"') {
            if (pos + 3 >= cap) {
                break;
            }
            out[pos++] = '\\';
            out[pos++] = '"';
        } else if (u == (unsigned char)'\n') {
            if (pos + 3 >= cap) {
                break;
            }
            out[pos++] = '\\';
            out[pos++] = 'n';
        } else if (u == (unsigned char)'\r') {
            if (pos + 3 >= cap) {
                break;
            }
            out[pos++] = '\\';
            out[pos++] = 'r';
        } else if (u == (unsigned char)'\t') {
            if (pos + 3 >= cap) {
                break;
            }
            out[pos++] = '\\';
            out[pos++] = 't';
        } else if (u < 0x20U) {
            int w = snprintf(out + pos, cap - pos, "\\u%04x", u);
            if (w <= 0 || (size_t)w >= cap - pos) {
                break;
            }
            pos += (size_t)w;
        } else {
            out[pos++] = (char)u;
        }
        ++in;
    }

    if (pos < cap) {
        out[pos] = '\0';
    } else if (cap > 0) {
        out[cap - 1] = '\0';
    }
}

void action_render_error(ActionResponse *res, const CortexError *err)
{
    char esc[CORTEX_ERR_MSG_MAX * 8];
    int n;

    if (!res || !err) {
        return;
    }

    action_error_json_escape(err->message[0] ? err->message : "error", esc, sizeof(esc));

    n = snprintf(action_error_tls, sizeof(action_error_tls),
                 "{\"error\":{\"code\":\"%s\",\"message\":\"%s\"}}", cortex_err_code_str(err->code),
                 esc);
    if (n < 0) {
        (void)snprintf(action_error_tls, sizeof(action_error_tls),
                       "{\"error\":{\"code\":\"UNKNOWN\",\"message\":\"serialization failed\"}}");
    }
    if (n >= (int)sizeof(action_error_tls)) {
        action_error_tls[sizeof(action_error_tls) - 1] = '\0';
    }

    cortex_err_print(err);
    action_response_set(res, cortex_err_to_http_status(err->code), action_error_tls);
    action_response_set_content_type(res, "application/json");
}

int action_handle_error(ActionResponse *res)
{
    if (!cortex_has_error()) {
        return 0;
    }
    action_render_error(res, cortex_last_error());
    cortex_clear_error();
    return 1;
}
