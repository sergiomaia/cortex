#include "db_migration_parser.h"

#include "../core/pulse.h"

#include <ctype.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MARKER_UP "-- migrate:up"
#define MARKER_DOWN "-- migrate:down"

void migration_sql_init(MigrationSQL *out) {
    if (!out) {
        return;
    }
    out->up_sql = NULL;
    out->down_sql = NULL;
}

void migration_sql_free(MigrationSQL *m) {
    if (!m) {
        return;
    }
    free(m->up_sql);
    free(m->down_sql);
    m->up_sql = NULL;
    m->down_sql = NULL;
}

static int is_marker_line(const char *line, const char *marker) {
    const char *p = line;

    if (!line || !marker) {
        return 0;
    }
    while (*p && isspace((unsigned char)*p)) {
        p++;
    }
    return strcmp(p, marker) == 0;
}

static int is_marker_line_len(const char *line, size_t len, const char *marker) {
    char stack_buf[80];

    if (!line || !marker || len >= sizeof(stack_buf)) {
        return 0;
    }
    memcpy(stack_buf, line, len);
    stack_buf[len] = '\0';
    return is_marker_line(stack_buf, marker);
}

typedef struct {
    char *data;
    size_t len;
    size_t cap;
} DynBuf;

static void dynbuf_init(DynBuf *b) {
    b->data = NULL;
    b->len = 0;
    b->cap = 0;
}

static void dynbuf_free(DynBuf *b) {
    free(b->data);
    dynbuf_init(b);
}

static int dynbuf_reserve(DynBuf *b, size_t extra) {
    size_t need;
    char *n;
    if (extra > SIZE_MAX - b->len) {
        return -1;
    }
    need = b->len + extra;
    if (need <= b->cap) {
        return 0;
    }
    if (b->cap == 0) {
        b->cap = 256;
    }
    while (b->cap < need) {
        if (b->cap > SIZE_MAX / 2) {
            return -1;
        }
        b->cap *= 2;
    }
    n = (char *)realloc(b->data, b->cap);
    if (!n) {
        return -1;
    }
    b->data = n;
    return 0;
}

static int dynbuf_append(DynBuf *b, const char *s, size_t n) {
    if (dynbuf_reserve(b, n) != 0) {
        return -1;
    }
    memcpy(b->data + b->len, s, n);
    b->len += n;
    return 0;
}

static char *read_entire_file(const char *filepath, size_t *out_len) {
    FILE *f;
    long sz;
    char *buf;

    if (!filepath || !out_len) {
        return NULL;
    }
    *out_len = 0;

    f = fopen(filepath, "rb");
    if (!f) {
        LOG_ERROR("db.migration.parser", "cannot open migration file '%s': %s", filepath, strerror(errno));
        return NULL;
    }
    if (fseek(f, 0, SEEK_END) != 0) {
        LOG_ERROR("db.migration.parser", "fseek failed on '%s'", filepath);
        fclose(f);
        return NULL;
    }
    sz = ftell(f);
    if (sz < 0) {
        LOG_ERROR("db.migration.parser", "ftell failed on '%s'", filepath);
        fclose(f);
        return NULL;
    }
    if (fseek(f, 0, SEEK_SET) != 0) {
        LOG_ERROR("db.migration.parser", "fseek rewind failed on '%s'", filepath);
        fclose(f);
        return NULL;
    }
    if ((size_t)sz > SIZE_MAX - 1) {
        fclose(f);
        LOG_ERROR("db.migration.parser", "migration file too large: '%s'", filepath);
        return NULL;
    }
    buf = (char *)malloc((size_t)sz + 1);
    if (!buf) {
        fclose(f);
        LOG_ERROR("db.migration.parser", "out of memory reading '%s'", filepath);
        return NULL;
    }
    if (sz > 0 && fread(buf, 1, (size_t)sz, f) != (size_t)sz) {
        free(buf);
        fclose(f);
        LOG_ERROR("db.migration.parser", "short read on '%s'", filepath);
        return NULL;
    }
    buf[sz] = '\0';
    fclose(f);
    *out_len = (size_t)sz;
    return buf;
}

typedef enum {
    PARSE_IDLE,
    PARSE_IN_UP,
    PARSE_IN_DOWN
} ParseSection;

int migration_sql_parse(const char *filepath, MigrationSQL *out) {
    char *whole = NULL;
    size_t whole_len = 0;
    const char *cursor;
    DynBuf up = {0};
    DynBuf dn = {0};
    ParseSection sec = PARSE_IDLE;
    int up_markers = 0;
    int down_markers = 0;
    int line_start = 1;

    if (!filepath || !out) {
        return -1;
    }
    migration_sql_init(out);

    whole = read_entire_file(filepath, &whole_len);
    if (!whole) {
        return -1;
    }

    if (whole_len == 0) {
        free(whole);
        LOG_ERROR("db.migration.parser", "empty migration file: '%s'", filepath);
        return -1;
    }

    cursor = whole;
    while (*cursor) {
        const char *line_end = strchr(cursor, '\n');
        size_t line_len;
        const char *line;
        int is_last_line = 0;

        if (!line_end) {
            line_end = cursor + strlen(cursor);
            is_last_line = 1;
        }
        line = cursor;
        line_len = (size_t)(line_end - line);
        {
            size_t mlen = line_len;
            while (mlen > 0 && (line[mlen - 1] == '\r' || line[mlen - 1] == '\n')) {
                mlen--;
            }
            if (line_start && is_marker_line_len(line, mlen, MARKER_UP)) {
                up_markers++;
                if (up_markers > 1) {
                    LOG_ERROR("db.migration.parser", "duplicate '%s' marker in '%s'", MARKER_UP, filepath);
                    goto fail;
                }
                if (sec == PARSE_IN_UP || sec == PARSE_IN_DOWN) {
                    LOG_ERROR("db.migration.parser", "nested '%s' marker in '%s'", MARKER_UP, filepath);
                    goto fail;
                }
                sec = PARSE_IN_UP;
            } else if (line_start && is_marker_line_len(line, mlen, MARKER_DOWN)) {
            down_markers++;
            if (down_markers > 1) {
                LOG_ERROR("db.migration.parser", "duplicate '%s' marker in '%s'", MARKER_DOWN, filepath);
                goto fail;
            }
            if (sec != PARSE_IN_UP) {
                LOG_ERROR("db.migration.parser", "'%s' must follow '%s' in '%s'", MARKER_DOWN, MARKER_UP, filepath);
                goto fail;
            }
                sec = PARSE_IN_DOWN;
            } else {
                if (sec == PARSE_IN_UP) {
                    if (dynbuf_append(&up, line, line_len) != 0) {
                        goto fail;
                    }
                    if (dynbuf_append(&up, "\n", 1) != 0) {
                        goto fail;
                    }
                } else if (sec == PARSE_IN_DOWN) {
                    if (dynbuf_append(&dn, line, line_len) != 0) {
                        goto fail;
                    }
                    if (dynbuf_append(&dn, "\n", 1) != 0) {
                        goto fail;
                    }
                }
            }
        }

        line_start = 1;
        if (is_last_line) {
            break;
        }
        cursor = line_end + 1;
    }

    if (up_markers == 0) {
        LOG_ERROR("db.migration.parser", "missing mandatory '%s' section in '%s'", MARKER_UP, filepath);
        goto fail;
    }

    /* Trim trailing whitespace from sections (keep internal newlines). */
    while (up.len > 0 && isspace((unsigned char)up.data[up.len - 1])) {
        up.len--;
    }
    while (dn.len > 0 && isspace((unsigned char)dn.data[dn.len - 1])) {
        dn.len--;
    }
    if (dynbuf_reserve(&up, 1) != 0) {
        goto fail;
    }
    up.data[up.len] = '\0';

    if (dn.len > 0) {
        if (dynbuf_reserve(&dn, 1) != 0) {
            goto fail;
        }
        dn.data[dn.len] = '\0';
    }

    if (up.len == 0) {
        LOG_ERROR("db.migration.parser", "empty '%s' body in '%s'", MARKER_UP, filepath);
        goto fail;
    }

    out->up_sql = up.data;
    up.data = NULL;
    if (dn.len > 0) {
        out->down_sql = dn.data;
        dn.data = NULL;
    } else {
        out->down_sql = NULL;
    }

    dynbuf_free(&up);
    dynbuf_free(&dn);
    free(whole);
    return 0;

fail:
    dynbuf_free(&up);
    dynbuf_free(&dn);
    migration_sql_free(out);
    free(whole);
    return -1;
}
