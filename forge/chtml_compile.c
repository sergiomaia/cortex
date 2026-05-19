/*
 * chtml_compile — build-time compiler for Cortex .chtml templates.
 *
 * Usage: chtml_compile input.chtml output.chtml.c function_name view_name
 */

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHTML_MAX_FILE     (8 * 1024 * 1024)
#define CHTML_LITERAL_MAX  (65536)
#define CHTML_TAG_MAX      (65536)
#define CHTML_ERR_MAX      512

typedef enum {
    MODE_TEXT,
    MODE_EXPR,
    MODE_RAW,
    MODE_CODE,
    MODE_COMMENT
} ParseMode;

typedef struct {
    FILE       *in;
    FILE       *out;
    const char *input_path;
    const char *function_name;
    const char *view_name;

    ParseMode   mode;
    char        literal[CHTML_LITERAL_MAX];
    int         literal_len;
    char        tag_buf[CHTML_TAG_MAX];
    int         tag_len;

    long        line;
    long        col;
    int         emitted;
    char        err[CHTML_ERR_MAX];
} Compiler;

static void compiler_set_error(Compiler *c, const char *msg) {
    if (!c) {
        return;
    }
    if (!msg) {
        msg = "unknown error";
    }
    snprintf(c->err, sizeof(c->err), "%s:%ld:%ld: %s",
             c->input_path ? c->input_path : "<input>",
             c->line, c->col, msg);
}

static int compiler_fail(Compiler *c, const char *msg) {
    compiler_set_error(c, msg);
    fprintf(stderr, "chtml_compile: %s\n", c->err);
    return 1;
}

static void compiler_track_char(Compiler *c, int ch) {
    if (!c) {
        return;
    }
    if (ch == '\n') {
        c->line++;
        c->col = 1;
    } else {
        c->col++;
    }
}

static int literal_append(Compiler *c, char ch) {
    if (!c) {
        return -1;
    }
    if (c->literal_len >= CHTML_LITERAL_MAX - 1) {
        compiler_set_error(c, "literal buffer overflow");
        return -1;
    }
    c->literal[c->literal_len++] = ch;
    c->literal[c->literal_len] = '\0';
    return 0;
}

static int tag_append(Compiler *c, char ch) {
    if (!c) {
        return -1;
    }
    if (c->tag_len >= CHTML_TAG_MAX - 1) {
        compiler_set_error(c, "tag buffer overflow");
        return -1;
    }
    c->tag_buf[c->tag_len++] = ch;
    c->tag_buf[c->tag_len] = '\0';
    return 0;
}

static void literal_reset(Compiler *c) {
    if (!c) {
        return;
    }
    c->literal_len = 0;
    c->literal[0] = '\0';
}

static void tag_reset(Compiler *c) {
    if (!c) {
        return;
    }
    c->tag_len = 0;
    c->tag_buf[0] = '\0';
}

static void trim_tag(Compiler *c) {
    int start;
    int end;

    if (!c || c->tag_len <= 0) {
        return;
    }

    start = 0;
    while (start < c->tag_len && isspace((unsigned char)c->tag_buf[start])) {
        start++;
    }

    end = c->tag_len - 1;
    while (end >= start && isspace((unsigned char)c->tag_buf[end])) {
        end--;
    }

    if (start > 0) {
        memmove(c->tag_buf, c->tag_buf + start, (size_t)(end - start + 1));
    }
    c->tag_len = end - start + 1;
    c->tag_buf[c->tag_len] = '\0';
}

static int emit_escaped_string(Compiler *c, const char *s, int len) {
    int i;

    if (!c || !c->out) {
        return -1;
    }
    if (fputc('"', c->out) == EOF) {
        return -1;
    }

    for (i = 0; i < len; i++) {
        unsigned char ch = (unsigned char)s[i];
        const char *esc = NULL;

        switch (ch) {
        case '\\':
            esc = "\\\\";
            break;
        case '"':
            esc = "\\\"";
            break;
        case '\n':
            esc = "\\n";
            break;
        case '\r':
            esc = "\\r";
            break;
        case '\t':
            esc = "\\t";
            break;
        default:
            if (fputc((int)ch, c->out) == EOF) {
                return -1;
            }
            continue;
        }

        if (esc && fputs(esc, c->out) == EOF) {
            return -1;
        }
    }

    if (fputc('"', c->out) == EOF) {
        return -1;
    }
    return 0;
}

static int flush_literal(Compiler *c) {
    if (!c || !c->out) {
        return -1;
    }
    if (c->literal_len <= 0) {
        return 0;
    }

    if (fputs("    cx_write(cx, ", c->out) == EOF) {
        return -1;
    }
    if (emit_escaped_string(c, c->literal, c->literal_len) != 0) {
        return -1;
    }
    if (fprintf(c->out, ", %d);\n", c->literal_len) < 0) {
        return -1;
    }

    c->emitted = 1;
    literal_reset(c);
    return 0;
}

static int flush_tag(Compiler *c) {
    if (!c || !c->out) {
        return -1;
    }

    trim_tag(c);

    switch (c->mode) {
    case MODE_EXPR:
        if (c->tag_len == 0) {
            return compiler_fail(c, "empty expression tag");
        }
        if (fprintf(c->out, "    cx_write_escaped(cx, %s);\n", c->tag_buf) < 0) {
            return -1;
        }
        c->emitted = 1;
        break;
    case MODE_RAW:
        if (c->tag_len == 0) {
            return compiler_fail(c, "empty raw expression tag");
        }
        if (fprintf(c->out, "    cx_write(cx, %s, -1);\n", c->tag_buf) < 0) {
            return -1;
        }
        c->emitted = 1;
        break;
    case MODE_CODE:
        if (c->tag_len > 0) {
            if (fprintf(c->out, "    %s\n", c->tag_buf) < 0) {
                return -1;
            }
            c->emitted = 1;
        }
        break;
    case MODE_COMMENT:
        break;
    default:
        return compiler_fail(c, "internal error: flush_tag in invalid mode");
    }

    tag_reset(c);
    return 0;
}

static int close_tag(Compiler *c) {
    int rc;

    rc = flush_tag(c);
    c->mode = MODE_TEXT;
    return rc;
}

/* Caller already consumed '%'; match optional '>' to close the tag. */
static int match_close_tag(Compiler *c) {
    int ch;

    ch = getc(c->in);
    if (ch == EOF) {
        return compiler_fail(c, "unexpected end of file inside tag (expected '%>')");
    }
    compiler_track_char(c, ch);

    if (ch != '>') {
        if (tag_append(c, '%') != 0) {
            return -1;
        }
        if (tag_append(c, (char)ch) != 0) {
            return -1;
        }
        return 0;
    }

    return close_tag(c);
}

static int begin_tag(Compiler *c, ParseMode mode) {
    if (flush_literal(c) != 0) {
        return -1;
    }
    tag_reset(c);
    c->mode = mode;
    return 0;
}

static int handle_lt(Compiler *c) {
    int ch2;
    int ch3;
    int ch4;

    ch2 = getc(c->in);
    if (ch2 == EOF) {
        if (literal_append(c, '<') != 0) {
            return -1;
        }
        return 0;
    }
    compiler_track_char(c, ch2);

    if (ch2 != '%') {
        if (literal_append(c, '<') != 0) {
            return -1;
        }
        if (literal_append(c, (char)ch2) != 0) {
            return -1;
        }
        return 0;
    }

    ch3 = getc(c->in);
    if (ch3 == EOF) {
        return compiler_fail(c, "unexpected end of file after '<%'");
    }
    compiler_track_char(c, ch3);

    if (ch3 == '=') {
        ch4 = getc(c->in);
        if (ch4 == EOF) {
            return compiler_fail(c, "unexpected end of file after '<%='");
        }
        compiler_track_char(c, ch4);

        if (ch4 == '=') {
            return begin_tag(c, MODE_RAW);
        }

        if (begin_tag(c, MODE_EXPR) != 0) {
            return -1;
        }
        return tag_append(c, (char)ch4);
    }

    if (ch3 == '#') {
        return begin_tag(c, MODE_COMMENT);
    }

    if (begin_tag(c, MODE_CODE) != 0) {
        return -1;
    }
    return tag_append(c, (char)ch3);
}

static int compile_stream(Compiler *c) {
    int ch;

    while ((ch = getc(c->in)) != EOF) {
        compiler_track_char(c, ch);

        if (c->mode == MODE_TEXT) {
            if (ch == '<') {
                if (handle_lt(c) != 0) {
                    return -1;
                }
            } else {
                if (literal_append(c, (char)ch) != 0) {
                    return -1;
                }
            }
            continue;
        }

        if (ch == '%') {
            if (match_close_tag(c) != 0) {
                return -1;
            }
            continue;
        }

        if (tag_append(c, (char)ch) != 0) {
            return -1;
        }
    }

    if (c->mode != MODE_TEXT) {
        return compiler_fail(c, "unclosed tag at end of file");
    }

    return flush_literal(c);
}

static int write_header(Compiler *c) {
    if (!c->out) {
        return -1;
    }

    if (fputs("/* Auto-generated by chtml_compile — do not edit. */\n", c->out) == EOF) {
        return -1;
    }
    if (fprintf(c->out, "/* Source: %s */\n\n", c->input_path) < 0) {
        return -1;
    }
    if (fputs("#include \"cx_context.h\"\n", c->out) == EOF) {
        return -1;
    }
    if (fputs("#include \"action_view.h\"\n\n", c->out) == EOF) {
        return -1;
    }
    if (fprintf(c->out, "static void %s(CxContext *cx)\n{\n", c->function_name) < 0) {
        return -1;
    }
    return 0;
}

static int write_footer(Compiler *c) {
    if (!c->out) {
        return -1;
    }

    if (!c->emitted) {
        if (fputs("    (void)cx;\n", c->out) == EOF) {
            return -1;
        }
    }

    if (fputs("}\n\n", c->out) == EOF) {
        return -1;
    }
    if (fprintf(c->out, "CORTEX_VIEW(\"%s\", %s)\n", c->view_name, c->function_name) < 0) {
        return -1;
    }
    return 0;
}

static int validate_identifiers(const char *function_name, const char *view_name) {
    const char *p;

    if (!function_name || function_name[0] == '\0') {
        fprintf(stderr, "chtml_compile: function name is required\n");
        return 1;
    }
    if (!view_name || view_name[0] == '\0') {
        fprintf(stderr, "chtml_compile: view name is required\n");
        return 1;
    }

    for (p = function_name; *p; p++) {
        if (!isalnum((unsigned char)*p) && *p != '_') {
            fprintf(stderr, "chtml_compile: invalid function name: %s\n", function_name);
            return 1;
        }
    }

    for (p = view_name; *p; p++) {
        if (!isalnum((unsigned char)*p) && *p != '_' && *p != '/') {
            fprintf(stderr, "chtml_compile: invalid view name: %s\n", view_name);
            return 1;
        }
    }

    return 0;
}

static long file_size(FILE *f) {
    long sz;

    if (fseek(f, 0, SEEK_END) != 0) {
        return -1;
    }
    sz = ftell(f);
    if (sz < 0) {
        return -1;
    }
    if (fseek(f, 0, SEEK_SET) != 0) {
        return -1;
    }
    return sz;
}

int main(int argc, char **argv) {
    Compiler c;
    const char *input_path;
    const char *output_path;
    FILE *in;
    FILE *out;
    long sz;

    if (argc != 5) {
        fprintf(stderr,
                "Usage: %s input.chtml output.chtml.c function_name view_name\n",
                argv[0]);
        return 2;
    }

    memset(&c, 0, sizeof(c));
    input_path = argv[1];
    output_path = argv[2];
    c.input_path = input_path;
    c.function_name = argv[3];
    c.view_name = argv[4];
    c.line = 1;
    c.col = 1;
    c.mode = MODE_TEXT;

    if (validate_identifiers(c.function_name, c.view_name) != 0) {
        return 1;
    }

    in = fopen(input_path, "rb");
    if (!in) {
        fprintf(stderr, "chtml_compile: cannot open input '%s': %s\n",
                input_path, strerror(errno));
        return 1;
    }

    sz = file_size(in);
    if (sz < 0) {
        fprintf(stderr, "chtml_compile: cannot determine size of '%s'\n", input_path);
        fclose(in);
        return 1;
    }
    if (sz > CHTML_MAX_FILE) {
        fprintf(stderr, "chtml_compile: template too large (%ld bytes)\n", sz);
        fclose(in);
        return 1;
    }

    out = fopen(output_path, "wb");
    if (!out) {
        fprintf(stderr, "chtml_compile: cannot open output '%s': %s\n",
                output_path, strerror(errno));
        fclose(in);
        return 1;
    }

    c.in = in;
    c.out = out;

    if (write_header(&c) != 0) {
        fprintf(stderr, "chtml_compile: failed writing output header\n");
        fclose(in);
        fclose(out);
        return 1;
    }

    if (compile_stream(&c) != 0) {
        fclose(in);
        fclose(out);
        return 1;
    }

    if (write_footer(&c) != 0) {
        fprintf(stderr, "chtml_compile: failed writing output footer\n");
        fclose(in);
        fclose(out);
        return 1;
    }

    fclose(in);
    fclose(out);
    return 0;
}
