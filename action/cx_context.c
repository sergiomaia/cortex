#include "cx_context.h"

#include <stdio.h>
#include <stdlib.h>

static int cx_buf_remaining(const CxContext *cx) {
    if (!cx) {
        return 0;
    }
    if (cx->buf_len < 0 || cx->buf_len >= CX_BUF_SIZE) {
        return 0;
    }
    return CX_BUF_SIZE - 1 - cx->buf_len;
}

static int cx_append_bytes(CxContext *cx, const char *s, int len) {
    int room;
    int n;

    if (!cx || !s) {
        return -1;
    }
    if (len < 0) {
        len = (int)strlen(s);
    }
    if (len == 0) {
        return 0;
    }

    room = cx_buf_remaining(cx);
    if (room <= 0) {
        return -1;
    }

    n = len;
    if (n > room) {
        n = room;
    }

    memcpy(cx->buf + cx->buf_len, s, (size_t)n);
    cx->buf_len += n;
    cx->buf[cx->buf_len] = '\0';

    if (n < len) {
        return -1;
    }
    return 0;
}

static int cx_find_var(const CxContext *cx, const char *key) {
    int i;

    if (!cx || !key || key[0] == '\0') {
        return -1;
    }

    for (i = 0; i < cx->var_count; i++) {
        if (strncmp(cx->vars[i].key, key, CX_KEY_MAX) == 0) {
            return i;
        }
    }
    return -1;
}

static int cx_key_valid(const char *key) {
    size_t n;

    if (!key) {
        return 0;
    }
    n = strlen(key);
    return n > 0 && n < CX_KEY_MAX;
}

void cx_init(CxContext *cx) {
    if (!cx) {
        return;
    }
    memset(cx, 0, sizeof(*cx));
    cx->layout = "application";
}

void cx_write(CxContext *cx, const char *s, int len) {
    if (!cx) {
        return;
    }
    if (!s) {
        return;
    }
    (void)cx_append_bytes(cx, s, len);
}

void cx_writef(CxContext *cx, const char *fmt, ...) {
    va_list ap;
    int room;
    int n;

    if (!cx || !fmt) {
        return;
    }

    room = cx_buf_remaining(cx);
    if (room <= 0) {
        return;
    }

    va_start(ap, fmt);
    n = vsnprintf(cx->buf + cx->buf_len, (size_t)room + 1, fmt, ap);
    va_end(ap);

    if (n < 0) {
        return;
    }
    if (n > room) {
        cx->buf_len = CX_BUF_SIZE - 1;
    } else {
        cx->buf_len += n;
    }
    cx->buf[cx->buf_len] = '\0';
}

static void cx_escape_and_write(CxContext *cx, const char *s) {
    const unsigned char *p;
    char chunk[8];
    int chunk_len;

    if (!cx) {
        return;
    }
    if (!s) {
        s = "";
    }

    for (p = (const unsigned char *)s; *p; p++) {
        const char *esc = NULL;
        chunk_len = 0;

        switch (*p) {
        case '&':
            esc = "&amp;";
            chunk_len = 5;
            break;
        case '"':
            esc = "&quot;";
            chunk_len = 6;
            break;
        case '\'':
            esc = "&#39;";
            chunk_len = 5;
            break;
        case '<':
            esc = "&lt;";
            chunk_len = 4;
            break;
        case '>':
            esc = "&gt;";
            chunk_len = 4;
            break;
        default:
            chunk[0] = (char)*p;
            chunk[1] = '\0';
            (void)cx_append_bytes(cx, chunk, 1);
            continue;
        }

        if (esc && chunk_len > 0) {
            (void)cx_append_bytes(cx, esc, chunk_len);
        }
    }
}

void cx_write_escaped(CxContext *cx, const char *s) {
    cx_escape_and_write(cx, s);
}

void cx_writef_escaped(CxContext *cx, const char *fmt, ...) {
    char tmp[CX_VAL_MAX];
    va_list ap;
    int n;

    if (!cx || !fmt) {
        return;
    }

    va_start(ap, fmt);
    n = vsnprintf(tmp, sizeof(tmp), fmt, ap);
    va_end(ap);

    if (n < 0) {
        return;
    }
    tmp[sizeof(tmp) - 1] = '\0';
    cx_escape_and_write(cx, tmp);
}

const char *cx_get(CxContext *cx, const char *key) {
    int idx;

    if (!cx || !key) {
        return NULL;
    }

    idx = cx_find_var(cx, key);
    if (idx < 0) {
        return NULL;
    }
    return cx->vars[idx].val;
}

int cx_get_int(CxContext *cx, const char *key) {
    int idx;

    if (!cx || !key) {
        return 0;
    }

    idx = cx_find_var(cx, key);
    if (idx < 0 || !cx->vars[idx].is_int) {
        return 0;
    }
    return cx->vars[idx].int_val;
}

int cx_isset(CxContext *cx, const char *key) {
    return cx_find_var(cx, key) >= 0 ? 1 : 0;
}

void cx_set(CxContext *cx, const char *key, const char *val) {
    int idx;
    CxVar *slot;

    if (!cx || !cx_key_valid(key)) {
        return;
    }
    if (!val) {
        val = "";
    }

    idx = cx_find_var(cx, key);
    if (idx >= 0) {
        slot = &cx->vars[idx];
    } else {
        if (cx->var_count >= CX_VAR_MAX) {
            return;
        }
        idx = cx->var_count++;
        slot = &cx->vars[idx];
        memset(slot, 0, sizeof(*slot));
    }

    strncpy(slot->key, key, CX_KEY_MAX - 1);
    slot->key[CX_KEY_MAX - 1] = '\0';
    strncpy(slot->val, val, CX_VAL_MAX - 1);
    slot->val[CX_VAL_MAX - 1] = '\0';
    slot->is_int = 0;
    slot->int_val = 0;
}

void cx_set_int(CxContext *cx, const char *key, int val) {
    int idx;
    CxVar *slot;
    char num[32];
    int n;

    if (!cx || !cx_key_valid(key)) {
        return;
    }

    idx = cx_find_var(cx, key);
    if (idx >= 0) {
        slot = &cx->vars[idx];
    } else {
        if (cx->var_count >= CX_VAR_MAX) {
            return;
        }
        idx = cx->var_count++;
        slot = &cx->vars[idx];
        memset(slot, 0, sizeof(*slot));
    }

    strncpy(slot->key, key, CX_KEY_MAX - 1);
    slot->key[CX_KEY_MAX - 1] = '\0';
    slot->is_int = 1;
    slot->int_val = val;

    n = snprintf(num, sizeof(num), "%d", val);
    if (n < 0) {
        num[0] = '\0';
    }
    strncpy(slot->val, num, CX_VAL_MAX - 1);
    slot->val[CX_VAL_MAX - 1] = '\0';
}

void cx_set_fmt(CxContext *cx, const char *key, const char *fmt, ...) {
    char tmp[CX_VAL_MAX];
    va_list ap;
    int n;

    if (!cx || !cx_key_valid(key) || !fmt) {
        return;
    }

    va_start(ap, fmt);
    n = vsnprintf(tmp, sizeof(tmp), fmt, ap);
    va_end(ap);

    if (n < 0) {
        return;
    }
    tmp[sizeof(tmp) - 1] = '\0';
    cx_set(cx, key, tmp);
}

void cx_set_layout(CxContext *cx, const char *layout) {
    if (!cx) {
        return;
    }
    cx->layout = layout;
}

const char *cx_yield(CxContext *cx) {
    if (!cx) {
        return "";
    }
    cx->yield_buf[cx->yield_len] = '\0';
    return cx->yield_buf;
}
