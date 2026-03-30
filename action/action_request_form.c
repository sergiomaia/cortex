#include "action_request_form.h"

#include <ctype.h>
#include <string.h>

static int hex_val(int c) {
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (c >= 'a' && c <= 'f') {
        return 10 + (c - 'a');
    }
    if (c >= 'A' && c <= 'F') {
        return 10 + (c - 'A');
    }
    return -1;
}

/* URL-decode src[len] into dst; returns decoded length, or -1 on overflow/invalid. */
static int url_decode(const char *src, size_t len, char *dst, size_t dst_cap) {
    size_t i = 0;
    size_t j = 0;

    while (i < len) {
        if (j + 1 >= dst_cap) {
            return -1;
        }
        if (src[i] == '+') {
            dst[j++] = ' ';
            ++i;
        } else if (src[i] == '%' && i + 2 < len) {
            int hi = hex_val((unsigned char)src[i + 1]);
            int lo = hex_val((unsigned char)src[i + 2]);
            if (hi < 0 || lo < 0) {
                return -1;
            }
            dst[j++] = (char)((hi << 4) | lo);
            i += 3;
        } else {
            dst[j++] = src[i++];
        }
    }
    if (j < dst_cap) {
        dst[j] = '\0';
    }
    return (int)j;
}

static int key_matches_encoded(const char *seg, size_t seg_len, const char *key) {
    char kbuf[256];
    int n;
    size_t key_len = strlen(key);

    if (seg_len == 0 || key_len == 0) {
        return 0;
    }
    /* Compare first segment=key: decode key part only for % in names (rare). */
    n = url_decode(seg, seg_len, kbuf, sizeof(kbuf));
    if (n < 0) {
        return 0;
    }
    return (size_t)n == key_len && memcmp(kbuf, key, key_len) == 0;
}

int action_request_form_get(const char *body, const char *key, char *out, size_t out_cap) {
    const char *p;
    const char *end;
    int presence_only = (out == NULL && out_cap == 0);

    if (!key || !key[0]) {
        return -1;
    }
    if (!presence_only && (!out || out_cap == 0)) {
        return -1;
    }
    if (!presence_only) {
        out[0] = '\0';
    }
    if (!body || !body[0]) {
        return -1;
    }

    p = body;
    end = body + strlen(body);

    while (p < end) {
        const char *amp = memchr(p, '&', (size_t)(end - p));
        const char *seg_end = amp ? amp : end;
        const char *eq = memchr(p, '=', (size_t)(seg_end - p));
        const char *val_start;
        size_t name_len;
        size_t val_len;
        int dec;

        if (!eq) {
            /* "flag" without = : treat as present with empty value for matching key */
            name_len = (size_t)(seg_end - p);
            val_start = seg_end;
            val_len = 0;
        } else {
            name_len = (size_t)(eq - p);
            val_start = eq + 1;
            val_len = (size_t)(seg_end - val_start);
        }

        if (key_matches_encoded(p, name_len, key)) {
            if (presence_only) {
                return 0;
            }
            dec = url_decode(val_start, val_len, out, out_cap);
            if (dec < 0) {
                return -1;
            }
            return dec;
        }

        p = seg_end;
        if (amp) {
            ++p;
        }
    }

    return -1;
}

int action_request_form_present(const char *body, const char *key) {
    return action_request_form_get(body, key, NULL, 0) >= 0 ? 1 : 0;
}
