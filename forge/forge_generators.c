#include "forge_generators.h"

#include "../core/core_secret.h"
#include "../db/db_create.h"

#include <ctype.h>
 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <sys/stat.h>
 #include <sys/types.h>
 #include <errno.h>
 #include <time.h>
 #include <stdarg.h>
 
static int ensure_javascript_structure(void);
static int ensure_app_assets_at(const char *project_root);
static int write_scaffold_react_resource_files(const char *resource,
                                               const char *resource_plural,
                                               int attr_count,
                                               const char **attributes);

typedef struct {
    char *p;
    size_t len;
    size_t cap;
} ForgeStrBuf;

static int forge_str_reserve(ForgeStrBuf *b, size_t add) {
    while (b->len + add + 1 > b->cap) {
        size_t ncap = b->cap ? b->cap * 2 : 16384;
        char *np = (char *)realloc(b->p, ncap);
        if (!np) {
            return -1;
        }
        b->p = np;
        b->cap = ncap;
    }
    return 0;
}

static int forge_str_fmt(ForgeStrBuf *b, const char *fmt, ...) {
    va_list ap;
    int n;

    va_start(ap, fmt);
    n = vsnprintf(NULL, 0, fmt, ap);
    va_end(ap);
    if (n < 0) {
        return -1;
    }
    if (forge_str_reserve(b, (size_t)n + 1) != 0) {
        return -1;
    }
    va_start(ap, fmt);
    vsnprintf(b->p + b->len, b->cap - b->len, fmt, ap);
    va_end(ap);
    b->len += (size_t)n;
    return 0;
}

static void forge_str_free(ForgeStrBuf *b) {
    free(b->p);
    b->p = NULL;
    b->len = 0;
    b->cap = 0;
}

static void str_to_lower(const char *in, char *out, size_t out_size);
static int normalize_resource_names(const char *input,
                                    char *resource_singular,
                                    size_t singular_size,
                                    char *resource_plural,
                                    size_t plural_size);
static int parse_attribute_key(const char *attr, char *out, size_t out_size);
static int parse_attribute_type(const char *attr, char *out, size_t out_size);

/* INSERT/UPDATE from form POST + redirect (see action_request_form + HTTP body in action_dispatch). */
static int forge_emit_scaffold_create_update(ForgeStrBuf *b,
                                             const char *resource_plural,
                                             int attr_count,
                                             const char **attributes) {
    int i;
    int bind_idx;

    if (attr_count == 0) {
        if (forge_str_fmt(b,
                          "void %s_create(ActionRequest *req, ActionResponse *res) {\n"
                          "    DbConnection *conn;\n"
                          "    DbStatement *st = NULL;\n"
                          "    char loc[128];\n"
                          "    const char *sql_ins = \"INSERT INTO %s DEFAULT VALUES\";\n"
                          "    (void)req;\n"
                          "    conn = cortex_db_connection();\n"
                          "    if (!conn) {\n"
                          "        action_controller_render_text(res, 500, \"Database unavailable\");\n"
                          "        return;\n"
                          "    }\n"
                          "    if (db_connection_prepare(conn, sql_ins, &st) != 0) {\n"
                          "        action_controller_render_text(res, 500, \"Database query failed\");\n"
                          "        return;\n"
                          "    }\n"
                          "    if (db_statement_step(st) != 0) {\n"
                          "        db_statement_finalize(st);\n"
                          "        action_controller_render_text(res, 500, \"Database write failed\");\n"
                          "        return;\n"
                          "    }\n"
                          "    db_statement_finalize(st);\n"
                          "    (void)snprintf(loc, sizeof(loc), \"/%%s\", \"%s\");\n"
                          "    action_controller_render_redirect(res, 303, loc);\n"
                          "}\n\n"
                          "void %s_update(ActionRequest *req, ActionResponse *res) {\n"
                          "    int rid = 0;\n"
                          "    char path_fmt[128];\n"
                          "    DbConnection *conn;\n"
                          "    DbStatement *st = NULL;\n"
                          "    char loc[128];\n"
                          "    const char *sql_up = \"UPDATE %s SET id=id WHERE id=?\";\n"
                          "    (void)snprintf(path_fmt, sizeof(path_fmt), \"/%%s/%%%%d\", \"%s\");\n"
                          "    if (sscanf(req->path, path_fmt, &rid) != 1) {\n"
                          "        action_controller_render_text(res, 400, \"Bad request\");\n"
                          "        return;\n"
                          "    }\n"
                          "    (void)req;\n"
                          "    conn = cortex_db_connection();\n"
                          "    if (!conn) {\n"
                          "        action_controller_render_text(res, 500, \"Database unavailable\");\n"
                          "        return;\n"
                          "    }\n"
                          "    if (db_connection_prepare(conn, sql_up, &st) != 0) {\n"
                          "        action_controller_render_text(res, 500, \"Database query failed\");\n"
                          "        return;\n"
                          "    }\n"
                          "    if (db_statement_bind_int(st, 1, rid) != 0) {\n"
                          "        db_statement_finalize(st);\n"
                          "        action_controller_render_text(res, 500, \"Bind failed\");\n"
                          "        return;\n"
                          "    }\n"
                          "    if (db_statement_step(st) != 0) {\n"
                          "        db_statement_finalize(st);\n"
                          "        action_controller_render_text(res, 500, \"Database write failed\");\n"
                          "        return;\n"
                          "    }\n"
                          "    db_statement_finalize(st);\n"
                          "    (void)snprintf(loc, sizeof(loc), \"/%%s\", \"%s\");\n"
                          "    action_controller_render_redirect(res, 303, loc);\n"
                          "}\n\n",
                          resource_plural,
                          resource_plural,
                          resource_plural,
                          resource_plural,
                          resource_plural,
                          resource_plural,
                          resource_plural) != 0) {
            return -1;
        }
        return 0;
    }

    if (forge_str_fmt(b,
                      "void %s_create(ActionRequest *req, ActionResponse *res) {\n"
                      "    DbConnection *conn;\n"
                      "    DbStatement *st = NULL;\n"
                      "    char val_buf[8192];\n"
                      "    char loc[128];\n"
                      "    const char *sql_ins = \"INSERT INTO %s (",
                      resource_plural,
                      resource_plural) != 0) {
        return -1;
    }

    for (i = 0; i < attr_count; ++i) {
        char key[64];
        char field[64];
        if (!attributes || !attributes[i]) {
            return -1;
        }
        if (parse_attribute_key(attributes[i], key, sizeof(key)) != 0) {
            return -1;
        }
        str_to_lower(key, field, sizeof(field));
        if (forge_str_fmt(b, "%s%s", i > 0 ? ", " : "", field) != 0) {
            return -1;
        }
    }

    if (forge_str_fmt(b, ") VALUES (") != 0) {
        return -1;
    }
    for (i = 0; i < attr_count; ++i) {
        if (forge_str_fmt(b, "%s?", i > 0 ? ", " : "") != 0) {
            return -1;
        }
    }

    if (forge_str_fmt(b,
                      ")\";\n"
                      "    if (!req || !req->body) {\n"
                      "        action_controller_render_text(res, 400, \"Bad request\");\n"
                      "        return;\n"
                      "    }\n"
                      "    conn = cortex_db_connection();\n"
                      "    if (!conn) {\n"
                      "        action_controller_render_text(res, 500, \"Database unavailable\");\n"
                      "        return;\n"
                      "    }\n"
                      "    if (db_connection_prepare(conn, sql_ins, &st) != 0) {\n"
                      "        action_controller_render_text(res, 500, \"Database query failed\");\n"
                      "        return;\n"
                      "    }\n") != 0) {
        return -1;
    }

    bind_idx = 1;
    for (i = 0; i < attr_count; ++i) {
        char key[64];
        char field[64];
        char typ[32];
        int is_bool;
        int is_int;
        int is_real;
        if (parse_attribute_key(attributes[i], key, sizeof(key)) != 0) {
            return -1;
        }
        str_to_lower(key, field, sizeof(field));
        if (parse_attribute_type(attributes[i], typ, sizeof(typ)) != 0) {
            return -1;
        }
        is_bool = (strcmp(typ, "boolean") == 0 || strcmp(typ, "bool") == 0);
        is_int = (strcmp(typ, "integer") == 0 || strcmp(typ, "int") == 0);
        is_real = (strcmp(typ, "float") == 0 || strcmp(typ, "decimal") == 0 || strcmp(typ, "real") == 0);

        if (is_bool) {
            if (forge_str_fmt(b,
                              "    if (db_statement_bind_int(st, %d, action_request_form_present(req->body, \"%s\") ? 1 : 0) != 0) {\n"
                              "        db_statement_finalize(st);\n"
                              "        action_controller_render_text(res, 500, \"Bind failed\");\n"
                              "        return;\n"
                              "    }\n",
                              bind_idx++,
                              field) != 0) {
                return -1;
            }
        } else if (is_int) {
            if (forge_str_fmt(b,
                              "    if (action_request_form_get(req->body, \"%s\", val_buf, sizeof(val_buf)) < 0) {\n"
                              "        val_buf[0] = '\\0';\n"
                              "    }\n"
                              "    if (db_statement_bind_int(st, %d, atoi(val_buf)) != 0) {\n"
                              "        db_statement_finalize(st);\n"
                              "        action_controller_render_text(res, 500, \"Bind failed\");\n"
                              "        return;\n"
                              "    }\n",
                              field,
                              bind_idx++) != 0) {
                return -1;
            }
        } else if (is_real) {
            if (forge_str_fmt(b,
                              "    if (action_request_form_get(req->body, \"%s\", val_buf, sizeof(val_buf)) < 0) {\n"
                              "        val_buf[0] = '\\0';\n"
                              "    }\n"
                              "    if (db_statement_bind_text(st, %d, val_buf) != 0) {\n"
                              "        db_statement_finalize(st);\n"
                              "        action_controller_render_text(res, 500, \"Bind failed\");\n"
                              "        return;\n"
                              "    }\n",
                              field,
                              bind_idx++) != 0) {
                return -1;
            }
        } else {
            if (forge_str_fmt(b,
                              "    if (action_request_form_get(req->body, \"%s\", val_buf, sizeof(val_buf)) < 0) {\n"
                              "        val_buf[0] = '\\0';\n"
                              "    }\n"
                              "    if (db_statement_bind_text(st, %d, val_buf) != 0) {\n"
                              "        db_statement_finalize(st);\n"
                              "        action_controller_render_text(res, 500, \"Bind failed\");\n"
                              "        return;\n"
                              "    }\n",
                              field,
                              bind_idx++) != 0) {
                return -1;
            }
        }
    }

    if (forge_str_fmt(b,
                      "    if (db_statement_step(st) != 0) {\n"
                      "        db_statement_finalize(st);\n"
                      "        action_controller_render_text(res, 500, \"Database write failed\");\n"
                      "        return;\n"
                      "    }\n"
                      "    db_statement_finalize(st);\n"
                      "    (void)snprintf(loc, sizeof(loc), \"/%%s\", \"%s\");\n"
                      "    action_controller_render_redirect(res, 303, loc);\n"
                      "}\n\n",
                      resource_plural) != 0) {
        return -1;
    }

    /* --- update --- */
    if (forge_str_fmt(b,
                      "void %s_update(ActionRequest *req, ActionResponse *res) {\n"
                      "    int rid = 0;\n"
                      "    char path_fmt[128];\n"
                      "    DbConnection *conn;\n"
                      "    DbStatement *st = NULL;\n"
                      "    char val_buf[8192];\n"
                      "    char loc[128];\n"
                      "    const char *sql_up = \"UPDATE %s SET ",
                      resource_plural,
                      resource_plural) != 0) {
        return -1;
    }

    for (i = 0; i < attr_count; ++i) {
        char key[64];
        char field[64];
        if (parse_attribute_key(attributes[i], key, sizeof(key)) != 0) {
            return -1;
        }
        str_to_lower(key, field, sizeof(field));
        if (forge_str_fmt(b, "%s%s=?", i > 0 ? ", " : "", field) != 0) {
            return -1;
        }
    }

    if (forge_str_fmt(b,
                      " WHERE id=?\";\n"
                      "    (void)snprintf(path_fmt, sizeof(path_fmt), \"/%%s/%%%%d\", \"%s\");\n"
                      "    if (sscanf(req->path, path_fmt, &rid) != 1) {\n"
                      "        action_controller_render_text(res, 400, \"Bad request\");\n"
                      "        return;\n"
                      "    }\n"
                      "    if (!req || !req->body) {\n"
                      "        action_controller_render_text(res, 400, \"Bad request\");\n"
                      "        return;\n"
                      "    }\n"
                      "    conn = cortex_db_connection();\n"
                      "    if (!conn) {\n"
                      "        action_controller_render_text(res, 500, \"Database unavailable\");\n"
                      "        return;\n"
                      "    }\n"
                      "    if (db_connection_prepare(conn, sql_up, &st) != 0) {\n"
                      "        action_controller_render_text(res, 500, \"Database query failed\");\n"
                      "        return;\n"
                      "    }\n",
                      resource_plural) != 0) {
        return -1;
    }

    bind_idx = 1;
    for (i = 0; i < attr_count; ++i) {
        char key[64];
        char field[64];
        char typ[32];
        int is_bool;
        int is_int;
        int is_real;
        if (parse_attribute_key(attributes[i], key, sizeof(key)) != 0) {
            return -1;
        }
        str_to_lower(key, field, sizeof(field));
        if (parse_attribute_type(attributes[i], typ, sizeof(typ)) != 0) {
            return -1;
        }
        is_bool = (strcmp(typ, "boolean") == 0 || strcmp(typ, "bool") == 0);
        is_int = (strcmp(typ, "integer") == 0 || strcmp(typ, "int") == 0);
        is_real = (strcmp(typ, "float") == 0 || strcmp(typ, "decimal") == 0 || strcmp(typ, "real") == 0);

        if (is_bool) {
            if (forge_str_fmt(b,
                              "    if (db_statement_bind_int(st, %d, action_request_form_present(req->body, \"%s\") ? 1 : 0) != 0) {\n"
                              "        db_statement_finalize(st);\n"
                              "        action_controller_render_text(res, 500, \"Bind failed\");\n"
                              "        return;\n"
                              "    }\n",
                              bind_idx++,
                              field) != 0) {
                return -1;
            }
        } else if (is_int) {
            if (forge_str_fmt(b,
                              "    if (action_request_form_get(req->body, \"%s\", val_buf, sizeof(val_buf)) < 0) {\n"
                              "        val_buf[0] = '\\0';\n"
                              "    }\n"
                              "    if (db_statement_bind_int(st, %d, atoi(val_buf)) != 0) {\n"
                              "        db_statement_finalize(st);\n"
                              "        action_controller_render_text(res, 500, \"Bind failed\");\n"
                              "        return;\n"
                              "    }\n",
                              field,
                              bind_idx++) != 0) {
                return -1;
            }
        } else if (is_real) {
            if (forge_str_fmt(b,
                              "    if (action_request_form_get(req->body, \"%s\", val_buf, sizeof(val_buf)) < 0) {\n"
                              "        val_buf[0] = '\\0';\n"
                              "    }\n"
                              "    if (db_statement_bind_text(st, %d, val_buf) != 0) {\n"
                              "        db_statement_finalize(st);\n"
                              "        action_controller_render_text(res, 500, \"Bind failed\");\n"
                              "        return;\n"
                              "    }\n",
                              field,
                              bind_idx++) != 0) {
                return -1;
            }
        } else {
            if (forge_str_fmt(b,
                              "    if (action_request_form_get(req->body, \"%s\", val_buf, sizeof(val_buf)) < 0) {\n"
                              "        val_buf[0] = '\\0';\n"
                              "    }\n"
                              "    if (db_statement_bind_text(st, %d, val_buf) != 0) {\n"
                              "        db_statement_finalize(st);\n"
                              "        action_controller_render_text(res, 500, \"Bind failed\");\n"
                              "        return;\n"
                              "    }\n",
                              field,
                              bind_idx++) != 0) {
                return -1;
            }
        }
    }

    if (forge_str_fmt(b,
                      "    if (db_statement_bind_int(st, %d, rid) != 0) {\n"
                      "        db_statement_finalize(st);\n"
                      "        action_controller_render_text(res, 500, \"Bind failed\");\n"
                      "        return;\n"
                      "    }\n"
                      "    if (db_statement_step(st) != 0) {\n"
                      "        db_statement_finalize(st);\n"
                      "        action_controller_render_text(res, 500, \"Database write failed\");\n"
                      "        return;\n"
                      "    }\n"
                      "    db_statement_finalize(st);\n"
                      "    (void)snprintf(loc, sizeof(loc), \"/%%s\", \"%s\");\n"
                      "    action_controller_render_redirect(res, 303, loc);\n"
                      "}\n\n",
                      bind_idx,
                      resource_plural) != 0) {
        return -1;
    }

    return 0;
}

/* Emit C source for scaffold controller: index/show read rows from SQLite; new/edit still use templates. */
static int forge_emit_scaffold_controller(ForgeStrBuf *b,
                                          const char *resource,
                                          const char *resource_plural,
                                          const char *type_name,
                                          int attr_count,
                                          const char **attributes) {
    int i;

    (void)resource;
    if (forge_str_fmt(b,
                      "/* Auto-generated scaffold controller: %s (index/show use SQLite via cortex_db_connection) */\n"
                      "#include \"action_controller.h\"\n"
                      "#include \"action_view.h\"\n"
                      "#include \"action_request_form.h\"\n"
                      "#include \"db/db_bootstrap.h\"\n"
                      "#include \"db/db_connection.h\"\n\n"
                      "#include <stdio.h>\n"
                      "#include <stdlib.h>\n"
                      "#include <string.h>\n\n",
                      resource_plural) != 0) {
        return -1;
    }

    /* --- index --- */
    if (forge_str_fmt(b,
                      "void %s_index(ActionRequest *req, ActionResponse *res) {\n"
                      "    DbConnection *conn;\n"
                      "    DbStatement *st = NULL;\n"
                      "    char *html;\n"
                      "    char *p;\n"
                      "    size_t len = 0;\n"
                      "    size_t cap = 262144;\n"
                      "    int any = 0;\n"
                      "    const char *sql = \"SELECT id",
                      resource_plural) != 0) {
        return -1;
    }

    for (i = 0; i < attr_count; ++i) {
        char key[64];
        char field[64];
        if (!attributes || !attributes[i]) {
            return -1;
        }
        if (parse_attribute_key(attributes[i], key, sizeof(key)) != 0) {
            return -1;
        }
        str_to_lower(key, field, sizeof(field));
        if (forge_str_fmt(b, ", %s", field) != 0) {
            return -1;
        }
    }

    if (forge_str_fmt(b,
                      " FROM %s ORDER BY id ASC\";\n"
                      "    (void)req;\n"
                      "    conn = cortex_db_connection();\n"
                      "    if (!conn) {\n"
                      "        action_controller_render_text(res, 500, \"Database unavailable\");\n"
                      "        return;\n"
                      "    }\n"
                      "    html = (char *)malloc(cap);\n"
                      "    if (!html) {\n"
                      "        action_controller_render_text(res, 500, \"Out of memory\");\n"
                      "        return;\n"
                      "    }\n"
                      "    p = html;\n"
                      "    len += (size_t)snprintf(p + len, cap - len, \"<h1>%%s</h1>\\n\", \"%s\");\n"
                      "    len += (size_t)snprintf(p + len, cap - len, \"<p><a href=\\\"/%%s/new\\\">New %%s</a></p>\\n\", \"%s\", \"%s\");\n"
                      "    len += (size_t)snprintf(p + len, cap - len, \"<table border=\\\"1\\\" cellpadding=\\\"6\\\" cellspacing=\\\"0\\\">\\n\");\n"
                      "    len += (size_t)snprintf(p + len, cap - len, \"<thead><tr><th>id</th>\");\n",
                      resource_plural,
                      type_name,
                      resource_plural,
                      type_name) != 0) {
        return -1;
    }

    for (i = 0; i < attr_count; ++i) {
        char key[64];
        char field[64];
        if (parse_attribute_key(attributes[i], key, sizeof(key)) != 0) {
            return -1;
        }
        str_to_lower(key, field, sizeof(field));
        if (forge_str_fmt(b,
                          "    len += (size_t)snprintf(p + len, cap - len, \"<th>%%s</th>\", \"%s\");\n",
                          field) != 0) {
            return -1;
        }
    }

    if (forge_str_fmt(b,
                      "    len += (size_t)snprintf(p + len, cap - len, \"</tr></thead><tbody>\\n\");\n"
                      "    if (db_connection_prepare(conn, sql, &st) != 0) {\n"
                      "        free(html);\n"
                      "        action_controller_render_text(res, 500, \"Database query failed\");\n"
                      "        return;\n"
                      "    }\n"
                      "    while (db_statement_step(st) == 1) {\n"
                      "        int rid = db_statement_column_int(st, 0);\n"
                      "        any = 1;\n"
                      "        len += (size_t)snprintf(p + len, cap - len,\n"
                      "            \"<tr><td><a href=\\\"/%%s/%%d\\\">%%d</a></td>\",\n"
                      "            \"%s\", rid, rid);\n",
                      resource_plural) != 0) {
        return -1;
    }

    for (i = 0; i < attr_count; ++i) {
        char key[64];
        char field[64];
        char typ[32];
        int is_bool;
        if (parse_attribute_key(attributes[i], key, sizeof(key)) != 0) {
            return -1;
        }
        str_to_lower(key, field, sizeof(field));
        if (parse_attribute_type(attributes[i], typ, sizeof(typ)) != 0) {
            return -1;
        }
        is_bool = (strcmp(typ, "boolean") == 0 || strcmp(typ, "bool") == 0);
        if (is_bool) {
            if (forge_str_fmt(b,
                              "        {\n"
                              "            int bv = db_statement_column_int(st, %d);\n"
                              "            len += (size_t)snprintf(p + len, cap - len, \"<td>%%s</td>\", bv ? \"Yes\" : \"No\");\n"
                              "        }\n",
                              i + 1) != 0) {
                return -1;
            }
        } else {
            if (forge_str_fmt(b,
                              "        {\n"
                              "            char *ex = action_view_escape_html(db_statement_column_text(st, %d));\n"
                              "            len += (size_t)snprintf(p + len, cap - len, \"<td>%%s</td>\", ex ? ex : \"\");\n"
                              "            free(ex);\n"
                              "        }\n",
                              i + 1) != 0) {
                return -1;
            }
        }
    }

    if (forge_str_fmt(b,
                      "        len += (size_t)snprintf(p + len, cap - len, \"</tr>\\n\");\n"
                      "        if (len >= cap - 4096) {\n"
                      "            db_statement_finalize(st);\n"
                      "            free(html);\n"
                      "            action_controller_render_text(res, 500, \"Response too large\");\n"
                      "            return;\n"
                      "        }\n"
                      "    }\n"
                      "    db_statement_finalize(st);\n"
                      "    if (!any) {\n"
                      "        len += (size_t)snprintf(p + len, cap - len,\n"
                      "            \"<tr><td colspan=\\\"%%d\\\">No records yet. <a href=\\\"/%%s/new\\\">Create one</a>.</td></tr>\\n\",\n"
                      "            %d, \"%s\");\n"
                      "    }\n"
                      "    len += (size_t)snprintf(p + len, cap - len, \"</tbody></table>\\n\");\n"
                      "    render_html(res, html);\n"
                      "}\n\n",
                      1 + attr_count,
                      resource_plural) != 0) {
        return -1;
    }

    /* --- show --- */
    if (forge_str_fmt(b,
                      "void %s_show(ActionRequest *req, ActionResponse *res) {\n"
                      "    int rid = 0;\n"
                      "    char path_fmt[128];\n"
                      "    DbConnection *conn;\n"
                      "    DbStatement *st = NULL;\n"
                      "    char *html;\n"
                      "    char *p;\n"
                      "    size_t len = 0;\n"
                      "    size_t cap = 65536;\n"
                      "    const char *sql_one = \"SELECT id",
                      resource_plural) != 0) {
        return -1;
    }

    for (i = 0; i < attr_count; ++i) {
        char key[64];
        char field[64];
        if (parse_attribute_key(attributes[i], key, sizeof(key)) != 0) {
            return -1;
        }
        str_to_lower(key, field, sizeof(field));
        if (forge_str_fmt(b, ", %s", field) != 0) {
            return -1;
        }
    }

    if (forge_str_fmt(b,
                      " FROM %s WHERE id = ?\";\n"
                      "    (void)snprintf(path_fmt, sizeof(path_fmt), \"/%%s/%%%%d\", \"%s\");\n"
                      "    if (sscanf(req->path, path_fmt, &rid) != 1) {\n"
                      "        action_controller_render_text(res, 400, \"Bad request\");\n"
                      "        return;\n"
                      "    }\n"
                      "    conn = cortex_db_connection();\n"
                      "    if (!conn) {\n"
                      "        action_controller_render_text(res, 500, \"Database unavailable\");\n"
                      "        return;\n"
                      "    }\n"
                      "    html = (char *)malloc(cap);\n"
                      "    if (!html) {\n"
                      "        action_controller_render_text(res, 500, \"Out of memory\");\n"
                      "        return;\n"
                      "    }\n"
                      "    p = html;\n"
                      "    if (db_connection_prepare(conn, sql_one, &st) != 0) {\n"
                      "        free(html);\n"
                      "        action_controller_render_text(res, 500, \"Database query failed\");\n"
                      "        return;\n"
                      "    }\n"
                      "    if (db_statement_bind_int(st, 1, rid) != 0) {\n"
                      "        db_statement_finalize(st);\n"
                      "        free(html);\n"
                      "        action_controller_render_text(res, 500, \"Bind failed\");\n"
                      "        return;\n"
                      "    }\n"
                      "    if (db_statement_step(st) != 1) {\n"
                      "        db_statement_finalize(st);\n"
                      "        free(html);\n"
                      "        action_controller_render_text(res, 404, \"Not found\");\n"
                      "        return;\n"
                      "    }\n"
                      "    len += (size_t)snprintf(p + len, cap - len,\n"
                      "        \"<h1>%%s</h1>\\n<dl>\\n\"\n"
                      "        \"  <dt>id</dt><dd>%%d</dd>\\n\",\n"
                      "        \"%s\", db_statement_column_int(st, 0));\n",
                      resource_plural,
                      resource_plural,
                      type_name) != 0) {
        return -1;
    }

    for (i = 0; i < attr_count; ++i) {
        char key[64];
        char field[64];
        char typ[32];
        int is_bool;
        if (parse_attribute_key(attributes[i], key, sizeof(key)) != 0) {
            return -1;
        }
        str_to_lower(key, field, sizeof(field));
        if (parse_attribute_type(attributes[i], typ, sizeof(typ)) != 0) {
            return -1;
        }
        is_bool = (strcmp(typ, "boolean") == 0 || strcmp(typ, "bool") == 0);
        if (is_bool) {
            if (forge_str_fmt(b,
                              "    len += (size_t)snprintf(p + len, cap - len,\n"
                              "        \"  <dt>%s</dt><dd>%%s</dd>\\n\",\n"
                              "        db_statement_column_int(st, %d) ? \"Yes\" : \"No\");\n",
                              field,
                              i + 1) != 0) {
                return -1;
            }
        } else {
            if (forge_str_fmt(b,
                              "    {\n"
                              "        char *ex = action_view_escape_html(db_statement_column_text(st, %d));\n"
                              "        len += (size_t)snprintf(p + len, cap - len,\n"
                              "            \"  <dt>%s</dt><dd>%%s</dd>\\n\",\n"
                              "            ex ? ex : \"\");\n"
                              "        free(ex);\n"
                              "    }\n",
                              i + 1,
                              field) != 0) {
                return -1;
            }
        }
    }

    if (forge_str_fmt(b,
                      "    len += (size_t)snprintf(p + len, cap - len,\n"
                      "        \"</dl>\\n<p><a href=\\\"/%s\\\">Back to list</a></p>\\n\");\n"
                      "    db_statement_finalize(st);\n"
                      "    render_html(res, html);\n"
                      "}\n\n",
                      resource_plural) != 0) {
        return -1;
    }

    /* new, edit; create/update from POST body; delete stub */
    if (forge_str_fmt(b,
                      "void %s_new(ActionRequest *req, ActionResponse *res) { (void)req; render_view(res, \"%s/new\"); }\n"
                      "void %s_edit(ActionRequest *req, ActionResponse *res) { (void)req; render_view(res, \"%s/edit\"); }\n",
                      resource_plural,
                      resource_plural,
                      resource_plural,
                      resource_plural) != 0) {
        return -1;
    }
    if (forge_emit_scaffold_create_update(b, resource_plural, attr_count, attributes) != 0) {
        return -1;
    }
    if (forge_str_fmt(b,
                      "void %s_delete(ActionRequest *req, ActionResponse *res) { (void)req; action_controller_render_text(res, 200, \"deleted\"); }\n",
                      resource_plural) != 0) {
        return -1;
    }

    return 0;
}

static int forge_emit_scaffold_controller_react(ForgeStrBuf *b,
                                                const char *resource_plural,
                                                int attr_count,
                                                const char **attributes) {
    int i;

    if (forge_str_fmt(
            b,
            "/* Auto-generated React scaffold controller: %s */\n"
            "#include \"action_controller.h\"\n"
            "#include \"action_view.h\"\n"
            "#include \"action_request_form.h\"\n"
            "#include \"action_response.h\"\n"
            "#include \"db/db_bootstrap.h\"\n"
            "#include \"db/db_connection.h\"\n\n"
            "#include <stdio.h>\n"
            "#include <stdlib.h>\n"
            "#include <string.h>\n\n"
            "static char *json_escape(const char *input) {\n"
            "    const unsigned char *p;\n"
            "    size_t cap = 1;\n"
            "    size_t len = 0;\n"
            "    char *out;\n"
            "    if (!input) input = \"\";\n"
            "    for (p = (const unsigned char *)input; *p; ++p) {\n"
            "        switch (*p) {\n"
            "        case '\\\\':\n"
            "        case '\"':\n"
            "            cap += 2;\n"
            "            break;\n"
            "        case '\\n':\n"
            "        case '\\r':\n"
            "        case '\\t':\n"
            "            cap += 2;\n"
            "            break;\n"
            "        default:\n"
            "            cap += 1;\n"
            "            break;\n"
            "        }\n"
            "    }\n"
            "    out = (char *)malloc(cap);\n"
            "    if (!out) return NULL;\n"
            "    for (p = (const unsigned char *)input; *p; ++p) {\n"
            "        switch (*p) {\n"
            "        case '\\\\': out[len++]='\\\\'; out[len++]='\\\\'; break;\n"
            "        case '\"': out[len++]='\\\\'; out[len++]='\"'; break;\n"
            "        case '\\n': out[len++]='\\\\'; out[len++]='n'; break;\n"
            "        case '\\r': out[len++]='\\\\'; out[len++]='r'; break;\n"
            "        case '\\t': out[len++]='\\\\'; out[len++]='t'; break;\n"
            "        default: out[len++]=(char)*p; break;\n"
            "        }\n"
            "    }\n"
            "    out[len] = '\\0';\n"
            "    return out;\n"
            "}\n\n",
            resource_plural) != 0) {
        return -1;
    }

    if (forge_str_fmt(
            b,
            "void %s_index(ActionRequest *req, ActionResponse *res) {\n"
            "    DbConnection *conn;\n"
            "    DbStatement *st = NULL;\n"
            "    char *json;\n"
            "    size_t len = 0;\n"
            "    size_t cap = 262144;\n"
            "    int first = 1;\n"
            "    const char *sql = \"SELECT id",
            resource_plural) != 0) {
        return -1;
    }
    for (i = 0; i < attr_count; ++i) {
        char key[64];
        char field[64];
        if (parse_attribute_key(attributes[i], key, sizeof(key)) != 0) return -1;
        str_to_lower(key, field, sizeof(field));
        if (forge_str_fmt(b, ", %s", field) != 0) return -1;
    }
    if (forge_str_fmt(
            b,
            " FROM %s ORDER BY id ASC\";\n"
            "    if (!req || !req->path) {\n"
            "        action_controller_render_text(res, 400, \"Bad request\");\n"
            "        return;\n"
            "    }\n"
            "    if (strstr(req->path, \".json\") == NULL) {\n"
            "        render_view(res, \"%s/index\");\n"
            "        return;\n"
            "    }\n"
            "    conn = cortex_db_connection();\n"
            "    if (!conn) {\n"
            "        action_controller_render_text(res, 500, \"Database unavailable\");\n"
            "        return;\n"
            "    }\n"
            "    json = (char *)malloc(cap);\n"
            "    if (!json) {\n"
            "        action_controller_render_text(res, 500, \"Out of memory\");\n"
            "        return;\n"
            "    }\n"
            "    len += (size_t)snprintf(json + len, cap - len, \"{\\\"data\\\":[\");\n"
            "    if (db_connection_prepare(conn, sql, &st) != 0) {\n"
            "        free(json);\n"
            "        action_controller_render_text(res, 500, \"Database query failed\");\n"
            "        return;\n"
            "    }\n"
            "    while (db_statement_step(st) == 1) {\n"
            "        if (!first) {\n"
            "            len += (size_t)snprintf(json + len, cap - len, \",\");\n"
            "        }\n"
            "        first = 0;\n"
            "        len += (size_t)snprintf(json + len, cap - len, \"{\\\"id\\\":%%d\", db_statement_column_int(st, 0));\n",
            resource_plural,
            resource_plural) != 0) {
        return -1;
    }
    for (i = 0; i < attr_count; ++i) {
        char key[64];
        char field[64];
        char typ[32];
        int is_bool;
        int is_int;
        int is_real;
        if (parse_attribute_key(attributes[i], key, sizeof(key)) != 0) return -1;
        str_to_lower(key, field, sizeof(field));
        if (parse_attribute_type(attributes[i], typ, sizeof(typ)) != 0) return -1;
        is_bool = (strcmp(typ, "boolean") == 0 || strcmp(typ, "bool") == 0);
        is_int = (strcmp(typ, "integer") == 0 || strcmp(typ, "int") == 0);
        is_real = (strcmp(typ, "float") == 0 || strcmp(typ, "decimal") == 0 || strcmp(typ, "real") == 0);
        if (is_bool) {
            if (forge_str_fmt(
                    b,
                    "        len += (size_t)snprintf(json + len, cap - len, \",\\\"%s\\\":%%s\", db_statement_column_int(st, %d) ? \"true\" : \"false\");\n",
                    field,
                    i + 1) != 0) return -1;
        } else if (is_int) {
            if (forge_str_fmt(
                    b,
                    "        len += (size_t)snprintf(json + len, cap - len, \",\\\"%s\\\":%%d\", db_statement_column_int(st, %d));\n",
                    field,
                    i + 1) != 0) return -1;
        } else if (is_real) {
            if (forge_str_fmt(
                    b,
                    "        {\n"
                    "            char *ev = json_escape(db_statement_column_text(st, %d));\n"
                    "            len += (size_t)snprintf(json + len, cap - len, \",\\\"%s\\\":%%s\", ev ? ev : \"0\");\n"
                    "            free(ev);\n"
                    "        }\n",
                    i + 1,
                    field) != 0) return -1;
        } else {
            if (forge_str_fmt(
                    b,
                    "        {\n"
                    "            char *ev = json_escape(db_statement_column_text(st, %d));\n"
                    "            len += (size_t)snprintf(json + len, cap - len, \",\\\"%s\\\":\\\"%%s\\\"\", ev ? ev : \"\");\n"
                    "            free(ev);\n"
                    "        }\n",
                    i + 1,
                    field) != 0) return -1;
        }
    }
    if (forge_str_fmt(
            b,
            "        len += (size_t)snprintf(json + len, cap - len, \"}\");\n"
            "        if (len >= cap - 4096) {\n"
            "            db_statement_finalize(st);\n"
            "            free(json);\n"
            "            action_controller_render_text(res, 500, \"Response too large\");\n"
            "            return;\n"
            "        }\n"
            "    }\n"
            "    db_statement_finalize(st);\n"
            "    len += (size_t)snprintf(json + len, cap - len, \"]}\");\n"
            "    action_controller_render_json(res, 200, json);\n"
            "    action_response_set_content_type(res, \"application/json\");\n"
            "}\n\n") != 0) {
        return -1;
    }

    if (forge_str_fmt(
            b,
            "void %s_show(ActionRequest *req, ActionResponse *res) {\n"
            "    int rid = 0;\n"
            "    char path_fmt[128];\n"
            "    char path_fmt_json[128];\n"
            "    DbConnection *conn;\n"
            "    DbStatement *st = NULL;\n"
            "    char *json;\n"
            "    size_t len = 0;\n"
            "    size_t cap = 32768;\n"
            "    const char *sql = \"SELECT id",
            resource_plural,
            resource_plural) != 0) {
        return -1;
    }
    for (i = 0; i < attr_count; ++i) {
        char key[64];
        char field[64];
        if (parse_attribute_key(attributes[i], key, sizeof(key)) != 0) return -1;
        str_to_lower(key, field, sizeof(field));
        if (forge_str_fmt(b, ", %s", field) != 0) return -1;
    }
    if (forge_str_fmt(
            b,
            " FROM %s WHERE id = ?\";\n"
            "    if (!req || !req->path) {\n"
            "        action_controller_render_text(res, 400, \"Bad request\");\n"
            "        return;\n"
            "    }\n"
            "    (void)snprintf(path_fmt, sizeof(path_fmt), \"/%%s/%%%%d\", \"%s\");\n"
            "    (void)snprintf(path_fmt_json, sizeof(path_fmt_json), \"/%%s/%%%%d.json\", \"%s\");\n"
            "    if (sscanf(req->path, path_fmt_json, &rid) != 1 && sscanf(req->path, path_fmt, &rid) != 1) {\n"
            "        action_controller_render_text(res, 400, \"Bad request\");\n"
            "        return;\n"
            "    }\n"
            "    if (strstr(req->path, \".json\") == NULL) {\n"
            "        render_view(res, \"%s/show\");\n"
            "        return;\n"
            "    }\n"
            "    conn = cortex_db_connection();\n"
            "    if (!conn) {\n"
            "        action_controller_render_text(res, 500, \"Database unavailable\");\n"
            "        return;\n"
            "    }\n"
            "    json = (char *)malloc(cap);\n"
            "    if (!json) {\n"
            "        action_controller_render_text(res, 500, \"Out of memory\");\n"
            "        return;\n"
            "    }\n"
            "    if (db_connection_prepare(conn, sql, &st) != 0) {\n"
            "        free(json);\n"
            "        action_controller_render_text(res, 500, \"Database query failed\");\n"
            "        return;\n"
            "    }\n"
            "    if (db_statement_bind_int(st, 1, rid) != 0) {\n"
            "        db_statement_finalize(st);\n"
            "        free(json);\n"
            "        action_controller_render_text(res, 500, \"Bind failed\");\n"
            "        return;\n"
            "    }\n"
            "    if (db_statement_step(st) != 1) {\n"
            "        db_statement_finalize(st);\n"
            "        free(json);\n"
            "        action_controller_render_text(res, 404, \"Not found\");\n"
            "        return;\n"
            "    }\n"
            "    len += (size_t)snprintf(json + len, cap - len, \"{\\\"data\\\":{\\\"id\\\":%%d\", db_statement_column_int(st, 0));\n",
            resource_plural,
            resource_plural,
            resource_plural,
            resource_plural,
            resource_plural) != 0) {
        return -1;
    }
    for (i = 0; i < attr_count; ++i) {
        char key[64];
        char field[64];
        char typ[32];
        int is_bool;
        int is_int;
        int is_real;
        if (parse_attribute_key(attributes[i], key, sizeof(key)) != 0) return -1;
        str_to_lower(key, field, sizeof(field));
        if (parse_attribute_type(attributes[i], typ, sizeof(typ)) != 0) return -1;
        is_bool = (strcmp(typ, "boolean") == 0 || strcmp(typ, "bool") == 0);
        is_int = (strcmp(typ, "integer") == 0 || strcmp(typ, "int") == 0);
        is_real = (strcmp(typ, "float") == 0 || strcmp(typ, "decimal") == 0 || strcmp(typ, "real") == 0);
        if (is_bool) {
            if (forge_str_fmt(
                    b,
                    "    len += (size_t)snprintf(json + len, cap - len, \",\\\"%s\\\":%%s\", db_statement_column_int(st, %d) ? \"true\" : \"false\");\n",
                    field,
                    i + 1) != 0) return -1;
        } else if (is_int) {
            if (forge_str_fmt(
                    b,
                    "    len += (size_t)snprintf(json + len, cap - len, \",\\\"%s\\\":%%d\", db_statement_column_int(st, %d));\n",
                    field,
                    i + 1) != 0) return -1;
        } else if (is_real) {
            if (forge_str_fmt(
                    b,
                    "    {\n"
                    "        char *ev = json_escape(db_statement_column_text(st, %d));\n"
                    "        len += (size_t)snprintf(json + len, cap - len, \",\\\"%s\\\":%%s\", ev ? ev : \"0\");\n"
                    "        free(ev);\n"
                    "    }\n",
                    i + 1,
                    field) != 0) return -1;
        } else {
            if (forge_str_fmt(
                    b,
                    "    {\n"
                    "        char *ev = json_escape(db_statement_column_text(st, %d));\n"
                    "        len += (size_t)snprintf(json + len, cap - len, \",\\\"%s\\\":\\\"%%s\\\"\", ev ? ev : \"\");\n"
                    "        free(ev);\n"
                    "    }\n",
                    i + 1,
                    field) != 0) return -1;
        }
    }
    if (forge_str_fmt(
            b,
            "    len += (size_t)snprintf(json + len, cap - len, \"}}\");\n"
            "    db_statement_finalize(st);\n"
            "    action_controller_render_json(res, 200, json);\n"
            "    action_response_set_content_type(res, \"application/json\");\n"
            "}\n\n",
            resource_plural) != 0) {
        return -1;
    }

    if (forge_str_fmt(
            b,
            "void %s_new(ActionRequest *req, ActionResponse *res) { (void)req; render_view(res, \"%s/new\"); }\n"
            "void %s_edit(ActionRequest *req, ActionResponse *res) { (void)req; render_view(res, \"%s/edit\"); }\n",
            resource_plural,
            resource_plural,
            resource_plural,
            resource_plural) != 0) {
        return -1;
    }
    if (forge_emit_scaffold_create_update(b, resource_plural, attr_count, attributes) != 0) {
        return -1;
    }
    if (forge_str_fmt(
            b,
            "void %s_delete(ActionRequest *req, ActionResponse *res) {\n"
            "    int rid = 0;\n"
            "    char path_fmt[128];\n"
            "    DbConnection *conn;\n"
            "    DbStatement *st = NULL;\n"
            "    const char *sql_del = \"DELETE FROM %s WHERE id = ?\";\n"
            "    (void)snprintf(path_fmt, sizeof(path_fmt), \"/%%s/%%%%d\", \"%s\");\n"
            "    if (!req || sscanf(req->path, path_fmt, &rid) != 1) {\n"
            "        action_controller_render_text(res, 400, \"Bad request\");\n"
            "        return;\n"
            "    }\n"
            "    conn = cortex_db_connection();\n"
            "    if (!conn) {\n"
            "        action_controller_render_text(res, 500, \"Database unavailable\");\n"
            "        return;\n"
            "    }\n"
            "    if (db_connection_prepare(conn, sql_del, &st) != 0) {\n"
            "        action_controller_render_text(res, 500, \"Database query failed\");\n"
            "        return;\n"
            "    }\n"
            "    if (db_statement_bind_int(st, 1, rid) != 0) {\n"
            "        db_statement_finalize(st);\n"
            "        action_controller_render_text(res, 500, \"Bind failed\");\n"
            "        return;\n"
            "    }\n"
            "    if (db_statement_step(st) != 0) {\n"
            "        db_statement_finalize(st);\n"
            "        action_controller_render_text(res, 500, \"Database write failed\");\n"
            "        return;\n"
            "    }\n"
            "    db_statement_finalize(st);\n"
            "    action_controller_render_text(res, 200, \"deleted\");\n"
            "}\n",
            resource_plural,
            resource_plural,
            resource_plural) != 0) {
        return -1;
    }

    return 0;
}

 static int ensure_dir(const char *path) {
     if (mkdir(path, 0755) == -1) {
         if (errno == EEXIST) {
             return 0;
         }
         return -1;
     }
     return 0;
 }
 
 static int write_template_file(const char *path, const char *contents) {
     FILE *f = fopen(path, "w");
     if (f == NULL) {
         return -1;
     }
 
     if (fputs(contents, f) < 0) {
         fclose(f);
         return -1;
     }
 
     if (fclose(f) != 0) {
         return -1;
     }
 
     return 0;
 }

 static int write_template_file_if_missing(const char *path, const char *contents) {
     FILE *f = fopen(path, "r");
     if (f) {
         fclose(f);
         return 0;
     }
     return write_template_file(path, contents);
 }

 static const char *default_application_css(void) {
     return "/*\n"
            " * Application-wide styles (Cortex).\n"
            " * Served at /assets/stylesheets/application.css\n"
            " */\n\n"
            "body {\n"
            "  margin: 0;\n"
            "}\n";
 }

 /* project_root NULL or empty: app/assets under cwd. Otherwise <root>/app/assets. */
 static int ensure_app_assets_at(const char *project_root) {
     char path[384];
     const char *css = default_application_css();
     int use_prefix = (project_root != NULL && project_root[0] != '\0');

     if (!use_prefix) {
         if (ensure_dir("app/assets") != 0) return -1;
         if (ensure_dir("app/assets/images") != 0) return -1;
         if (ensure_dir("app/assets/stylesheets") != 0) return -1;
         return write_template_file_if_missing("app/assets/stylesheets/application.css", css);
     }

     if (snprintf(path, sizeof(path), "%s/app/assets", project_root) < 0) return -1;
     if (ensure_dir(path) != 0) return -1;
     if (snprintf(path, sizeof(path), "%s/app/assets/images", project_root) < 0) return -1;
     if (ensure_dir(path) != 0) return -1;
     if (snprintf(path, sizeof(path), "%s/app/assets/stylesheets", project_root) < 0) return -1;
     if (ensure_dir(path) != 0) return -1;
     if (snprintf(path, sizeof(path), "%s/app/assets/stylesheets/application.css", project_root) < 0) return -1;
     return write_template_file(path, css);
 }
 
 int forge_generate_controller(const char *name) {
    char resource[64];
    char resource_plural[64];
     char path[256];
     char buffer[512];
 
    if (!name || name[0] == '\0') return -1;
    if (normalize_resource_names(name, resource, sizeof(resource), resource_plural, sizeof(resource_plural)) != 0) return -1;

     if (ensure_dir("app") != 0) return -1;
     if (ensure_dir("app/controllers") != 0) return -1;
 
    if (snprintf(path, sizeof(path), "app/controllers/%s_controller.c", resource_plural) < 0) {
         return -1;
     }
 
     /* Minimal controller template. */
     if (snprintf(buffer, sizeof(buffer),
                  "/* Auto‑generated controller: %s */\n"
                  "#include \"action_controller.h\"\n\n"
                  "void %s_controller_handle(ActionRequest *req, ActionResponse *res) {\n"
                  "    (void)req;\n"
                  "    action_controller_render_text(res, 200, \"ok\");\n"
                  "}\n",
                 resource_plural, resource_plural) < 0) {
         return -1;
     }
 
     return write_template_file(path, buffer);
 }
 
int forge_generate_resource(const char *name) {
    char resource[64];
    char resource_plural[64];
    char path[256];
    char controller_buf[2048];

    if (!name || name[0] == '\0') return -1;
    if (normalize_resource_names(name, resource, sizeof(resource), resource_plural, sizeof(resource_plural)) != 0) return -1;

    if (ensure_dir("app") != 0) return -1;
    if (ensure_dir("app/controllers") != 0) return -1;
    if (ensure_dir("app/views") != 0) return -1;
    if (snprintf(path, sizeof(path), "app/views/%s", resource_plural) < 0) return -1;
    if (ensure_dir(path) != 0) return -1;

    if (snprintf(path, sizeof(path), "app/controllers/%s_controller.c", resource_plural) < 0) {
        return -1;
    }
    if (snprintf(controller_buf, sizeof(controller_buf),
                 "/* Auto-generated resource controller: %s */\n"
                 "#include \"action/action_controller.h\"\n\n"
                 "void %s_index(ActionRequest *req, ActionResponse *res) {\n"
                 "    (void)req;\n"
                 "    action_controller_render_text(res, 200, \"%s index\");\n"
                 "}\n\n"
                 "void %s_show(ActionRequest *req, ActionResponse *res) {\n"
                 "    (void)req;\n"
                 "    action_controller_render_text(res, 200, \"%s show\");\n"
                 "}\n\n"
                 "void %s_new(ActionRequest *req, ActionResponse *res) {\n"
                 "    (void)req;\n"
                 "    action_controller_render_text(res, 200, \"%s new\");\n"
                 "}\n\n"
                 "void %s_edit(ActionRequest *req, ActionResponse *res) {\n"
                 "    (void)req;\n"
                 "    action_controller_render_text(res, 200, \"%s edit\");\n"
                 "}\n\n"
                 "void %s_create(ActionRequest *req, ActionResponse *res) {\n"
                 "    (void)req;\n"
                 "    action_controller_render_text(res, 200, \"%s create\");\n"
                 "}\n\n"
                 "void %s_update(ActionRequest *req, ActionResponse *res) {\n"
                 "    (void)req;\n"
                 "    action_controller_render_text(res, 200, \"%s update\");\n"
                 "}\n\n"
                 "void %s_delete(ActionRequest *req, ActionResponse *res) {\n"
                 "    (void)req;\n"
                 "    action_controller_render_text(res, 200, \"%s delete\");\n"
                 "}\n",
                 resource_plural,
                 resource_plural, resource_plural,
                 resource_plural, resource_plural,
                 resource_plural, resource_plural,
                 resource_plural, resource_plural,
                 resource_plural, resource_plural,
                 resource_plural, resource_plural,
                 resource_plural, resource_plural) < 0) {
        return -1;
    }
    if (write_template_file(path, controller_buf) != 0) return -1;

    if (snprintf(path, sizeof(path), "app/views/%s/index.html", resource_plural) < 0) return -1;
    if (write_template_file(path, "<h1>Index</h1>\n<p>List all records here.</p>\n") != 0) return -1;

    if (snprintf(path, sizeof(path), "app/views/%s/show.html", resource_plural) < 0) return -1;
    if (write_template_file(path, "<h1>Show</h1>\n<p>Display one record here.</p>\n") != 0) return -1;

    if (snprintf(path, sizeof(path), "app/views/%s/new.html", resource_plural) < 0) return -1;
    if (write_template_file(path, "<h1>New</h1>\n<form method=\"POST\" action=\"\">\n  <button type=\"submit\">Create</button>\n</form>\n") != 0) return -1;

    if (snprintf(path, sizeof(path), "app/views/%s/edit.html", resource_plural) < 0) return -1;
    if (write_template_file(path, "<h1>Edit</h1>\n<form method=\"POST\" action=\"\">\n  <input type=\"hidden\" name=\"_method\" value=\"PUT\" />\n  <button type=\"submit\">Update</button>\n</form>\n") != 0) return -1;

    return 0;
}

 /* Copy name and capitalize first letter for type name (e.g. user -> User). */
 static void model_type_name(const char *name, char *type_buf, size_t type_size) {
     if (!name || !type_buf || type_size == 0) return;
     if (type_size > 1 && name[0]) {
         type_buf[0] = (char)toupper((unsigned char)name[0]);
         (void)snprintf(type_buf + 1, type_size - 1, "%s", name[1] ? name + 1 : "");
     } else if (type_size > 0) {
         type_buf[0] = '\0';
     }
 }
 
 /* Uppercase name for macro (e.g. user -> USER). */
 static void model_macro_name(const char *name, char *macro_buf, size_t macro_size) {
     size_t i;
     if (!name || !macro_buf || macro_size == 0) return;
     for (i = 0; i < macro_size - 1 && name[i]; i++) {
         macro_buf[i] = (char)toupper((unsigned char)name[i]);
     }
     macro_buf[i] = '\0';
 }
 
 int forge_generate_model(const char *name) {
    char resource[64];
    char resource_plural[64];
     char path[256];
     char type_name[64];
     char macro_name[64];
     char buffer[768];
 
     if (!name || name[0] == '\0') return -1;
    if (normalize_resource_names(name, resource, sizeof(resource), resource_plural, sizeof(resource_plural)) != 0) return -1;
     if (ensure_dir("app") != 0) return -1;
     if (ensure_dir("app/models") != 0) return -1;
 
    if (snprintf(path, sizeof(path), "app/models/%s.c", resource) < 0) {
         return -1;
     }
 
    model_type_name(resource, type_name, sizeof(type_name));
    model_macro_name(resource, macro_name, sizeof(macro_name));
 
     /* Struct (typedef ActiveModel), model name constant, ActiveRecord create. */
     if (snprintf(buffer, sizeof(buffer),
                  "/* Auto-generated model: %s */\n"
                  "#include \"core/active_record.h\"\n"
                  "#include \"core/active_model.h\"\n\n"
                  "#define %s_MODEL_NAME \"%s\"\n\n"
                  "typedef ActiveModel %s;\n\n"
                  "/* Fields: use active_model_set_field(m, \"key\", \"value\"); */\n\n"
                  "%s *%s_create(ActiveRecordStore *store) {\n"
                  "    return active_record_create(store, %s_MODEL_NAME);\n"
                  "}\n",
                 resource, macro_name, resource, type_name, type_name, resource, macro_name) < 0) {
         return -1;
     }
 
     return write_template_file(path, buffer);
 }

int forge_generate_service(const char *name) {
    char path[256];
    char buffer[512];

    if (!name || name[0] == '\0') return -1;
    if (ensure_dir("service") != 0) return -1;
    if (snprintf(path, sizeof(path), "service/%s.c", name) < 0) return -1;
    if (snprintf(buffer, sizeof(buffer),
                 "/* Auto-generated service: %s */\n"
                 "#include <stdio.h>\n\n"
                 "void %s_service_run(void) {\n"
                 "    /* TODO: implement %s service behavior. */\n"
                 "    (void)printf(\"%s service running\\n\");\n"
                 "}\n",
                 name, name, name, name) < 0) {
        return -1;
    }
    return write_template_file(path, buffer);
}
 
static int forge_neural_resource_name(const char *name, char *resource, size_t resource_size) {
    char resource_plural[64];
    if (!name || !resource || resource_size == 0) return -1;
    if (normalize_resource_names(name, resource, resource_size, resource_plural, sizeof(resource_plural)) != 0) return -1;
    return 0;
}

int forge_generate_neural_model(const char *name) {
    char resource[64];
    char path[256];
    char buffer[2048];

    if (forge_neural_resource_name(name, resource, sizeof(resource)) != 0) return -1;
    if (ensure_dir("app") != 0) return -1;
    if (ensure_dir("app/neural") != 0) return -1;
    if (snprintf(path, sizeof(path), "app/neural/%s_neural_model.c", resource) < 0) return -1;

    if (snprintf(buffer, sizeof(buffer),
                 "/* Auto-generated neural model: %s */\n"
                 "#include \"core/neural_model.h\"\n\n"
                 "static const char *%s_system_prompt =\n"
                 "    \"You are an assistant focused on %s operations.\";\n\n"
                 "const char *%s_neural_system_prompt(void) {\n"
                 "    return %s_system_prompt;\n"
                 "}\n\n"
                 "void %s_neural_enrich(NeuralModel *model) {\n"
                 "    (void)model;\n"
                 "    /* TODO: configure provider/model settings. */\n"
                 "    /* TODO: enrich context with application data. */\n"
                 "}\n",
                 resource,
                 resource,
                 resource,
                 resource,
                 resource,
                 resource,
                 resource,
                 resource) < 0) {
        return -1;
    }

    return write_template_file(path, buffer);
}

int forge_generate_neural_prompt(const char *name) {
    char resource[64];
    char path[256];
    char buffer[2048];

    if (forge_neural_resource_name(name, resource, sizeof(resource)) != 0) return -1;
    if (ensure_dir("app") != 0) return -1;
    if (ensure_dir("app/neural") != 0) return -1;
    if (ensure_dir("app/neural/prompts") != 0) return -1;
    if (snprintf(path, sizeof(path), "app/neural/prompts/%s_prompt.c", resource) < 0) return -1;

    if (snprintf(buffer, sizeof(buffer),
                 "/* Auto-generated neural prompt module: %s */\n"
                 "#include \"core/neural_prompt.h\"\n\n"
                 "static const char *%s_prompt_template =\n"
                 "    \"Task: {{task}}\\n\"\n"
                 "    \"Input: {{input}}\\n\"\n"
                 "    \"Context: {{context}}\\n\";\n\n"
                 "int %s_prompt_build(const NeuralPromptVar *vars,\n"
                 "                   int var_count,\n"
                 "                   char *out,\n"
                 "                   int out_size) {\n"
                 "    return neural_prompt_render(%s_prompt_template, vars, var_count, out, out_size);\n"
                 "}\n",
                 resource,
                 resource,
                 resource,
                 resource) < 0) {
        return -1;
    }

    return write_template_file(path, buffer);
}

int forge_generate_neural_agent(const char *name) {
    char resource[64];
    char path[256];
    char buffer[3072];

    if (forge_neural_resource_name(name, resource, sizeof(resource)) != 0) return -1;
    if (ensure_dir("app") != 0) return -1;
    if (ensure_dir("app/neural") != 0) return -1;
    if (ensure_dir("app/neural/agents") != 0) return -1;
    if (snprintf(path, sizeof(path), "app/neural/agents/%s_agent.c", resource) < 0) return -1;

    if (snprintf(buffer, sizeof(buffer),
                 "/* Auto-generated neural agent module: %s */\n"
                 "#include \"core/neural_agent.h\"\n\n"
                 "static const char *%s_tool_help(const char *input) {\n"
                 "    (void)input;\n"
                 "    return \"help\";\n"
                 "}\n\n"
                 "int %s_agent_bootstrap(NeuralAgent *agent) {\n"
                 "    if (!agent) return -1;\n"
                 "    neural_agent_init(agent);\n"
                 "    if (neural_agent_register_tool(agent, \"help\", \"help\", %s_tool_help) != 0) {\n"
                 "        neural_agent_free(agent);\n"
                 "        return -1;\n"
                 "    }\n"
                 "    return 0;\n"
                 "}\n\n"
                 "const char *%s_agent_run(NeuralAgent *agent, const char *input) {\n"
                 "    return neural_agent_call_tool(agent, input);\n"
                 "}\n",
                 resource,
                 resource,
                 resource,
                 resource,
                 resource) < 0) {
        return -1;
    }

    return write_template_file(path, buffer);
}

int forge_generate_neural_rag(const char *name) {
    char resource[64];
    char path[256];
    char buffer[3072];

    if (forge_neural_resource_name(name, resource, sizeof(resource)) != 0) return -1;
    if (ensure_dir("app") != 0) return -1;
    if (ensure_dir("app/neural") != 0) return -1;
    if (ensure_dir("app/neural/rag") != 0) return -1;
    if (snprintf(path, sizeof(path), "app/neural/rag/%s_rag.c", resource) < 0) return -1;

    if (snprintf(buffer, sizeof(buffer),
                 "/* Auto-generated neural RAG module: %s */\n"
                 "#include \"core/rag_integration.h\"\n\n"
                 "void %s_rag_init(RAGEngine *engine) {\n"
                 "    if (!engine) return;\n"
                 "    rag_engine_init(engine, \"local-mock\");\n"
                 "}\n\n"
                 "int %s_rag_seed(RAGEngine *engine, const char *keyword, const char *value) {\n"
                 "    if (!engine || !keyword || !value) return -1;\n"
                 "    return rag_engine_store(engine, keyword, value);\n"
                 "}\n\n"
                 "int %s_rag_ask(RAGEngine *engine, const char *question, char *out, int out_size) {\n"
                 "    if (!engine || !question || !out || out_size <= 0) return -1;\n"
                 "    return rag_engine_query(engine, question, out, out_size);\n"
                 "}\n",
                 resource,
                 resource,
                 resource,
                 resource) < 0) {
        return -1;
    }

    return write_template_file(path, buffer);
}

int forge_generate_neural_stream(const char *name) {
    char resource[64];
    char path[256];
    char buffer[3072];

    if (forge_neural_resource_name(name, resource, sizeof(resource)) != 0) return -1;
    if (ensure_dir("app") != 0) return -1;
    if (ensure_dir("app/neural") != 0) return -1;
    if (ensure_dir("app/neural/streams") != 0) return -1;
    if (snprintf(path, sizeof(path), "app/neural/streams/%s_stream.c", resource) < 0) return -1;

    if (snprintf(buffer, sizeof(buffer),
                 "/* Auto-generated neural stream module: %s */\n"
                 "#include \"core/neural_stream.h\"\n\n"
                 "typedef struct {\n"
                 "    int tokens;\n"
                 "} %s_stream_state;\n\n"
                 "static void %s_stream_on_token(const char *token, int is_last, void *user_data) {\n"
                 "    %s_stream_state *state = (%s_stream_state *)user_data;\n"
                 "    (void)token;\n"
                 "    (void)is_last;\n"
                 "    if (state) state->tokens += 1;\n"
                 "}\n\n"
                 "int %s_stream_run(const char *prompt) {\n"
                 "    NeuralRuntime runtime;\n"
                 "    NeuralStream stream;\n"
                 "    %s_stream_state state;\n"
                 "    state.tokens = 0;\n"
                 "    runtime = neural_runtime_init(\"local-mock\");\n"
                 "    stream = neural_stream_init(runtime);\n"
                 "    return neural_stream_run(&stream, prompt, %s_stream_on_token, &state);\n"
                 "}\n",
                 resource,
                 resource,
                 resource,
                 resource,
                 resource,
                 resource,
                 resource,
                 resource) < 0) {
        return -1;
    }

    return write_template_file(path, buffer);
}

int forge_generate_neural_memory(const char *name) {
    char resource[64];
    char path[256];
    char buffer[2048];

    if (forge_neural_resource_name(name, resource, sizeof(resource)) != 0) return -1;
    if (ensure_dir("app") != 0) return -1;
    if (ensure_dir("app/neural") != 0) return -1;
    if (ensure_dir("app/neural/memory") != 0) return -1;
    if (snprintf(path, sizeof(path), "app/neural/memory/%s_memory.c", resource) < 0) return -1;

    if (snprintf(buffer, sizeof(buffer),
                 "/* Auto-generated neural memory module: %s */\n"
                 "#include \"core/neural_memory.h\"\n\n"
                 "int %s_memory_remember(NeuralMemory *memory, const char *key, const char *value) {\n"
                 "    if (!memory || !key || !value) return -1;\n"
                 "    return neural_memory_store(memory, key, value);\n"
                 "}\n\n"
                 "const char *%s_memory_recall(const NeuralMemory *memory, const char *key) {\n"
                 "    if (!memory || !key) return NULL;\n"
                 "    return neural_memory_retrieve(memory, key);\n"
                 "}\n",
                 resource,
                 resource,
                 resource) < 0) {
        return -1;
    }

    return write_template_file(path, buffer);
}

int forge_generate_neural_retriever(const char *name) {
    char resource[64];
    char path[256];
    char buffer[3072];

    if (forge_neural_resource_name(name, resource, sizeof(resource)) != 0) return -1;
    if (ensure_dir("app") != 0) return -1;
    if (ensure_dir("app/neural") != 0) return -1;
    if (ensure_dir("app/neural/retrievers") != 0) return -1;
    if (snprintf(path, sizeof(path), "app/neural/retrievers/%s_retriever.c", resource) < 0) return -1;

    if (snprintf(buffer, sizeof(buffer),
                 "/* Auto-generated neural retriever module: %s */\n"
                 "#include \"core/neural_retrieval.h\"\n"
                 "#include \"core/neural_embedding.h\"\n\n"
                 "int %s_retriever_index_text(NeuralRetrieval *retrieval,\n"
                 "                           const char *keyword,\n"
                 "                           const char *text) {\n"
                 "    NeuralEmbedding embedding;\n"
                 "    if (!retrieval || !keyword || !text) return -1;\n"
                 "    embedding = neural_embedding_from_text(text);\n"
                 "    return neural_retrieval_store(retrieval, keyword, embedding);\n"
                 "}\n\n"
                 "int %s_retriever_search(NeuralRetrieval *retrieval,\n"
                 "                        const char *query,\n"
                 "                        int top_k,\n"
                 "                        const char **out_keywords,\n"
                 "                        float *out_scores) {\n"
                 "    NeuralEmbedding query_embedding;\n"
                 "    if (!retrieval || !query) return -1;\n"
                 "    query_embedding = neural_embedding_from_text(query);\n"
                 "    return neural_retrieval_search(retrieval, query_embedding, top_k, out_keywords, out_scores);\n"
                 "}\n",
                 resource,
                 resource,
                 resource) < 0) {
        return -1;
    }

    return write_template_file(path, buffer);
}

int forge_generate_neural_integration(const char *name) {
    char resource[64];
    char path[256];
    char buffer[3072];

    if (forge_neural_resource_name(name, resource, sizeof(resource)) != 0) return -1;
    if (ensure_dir("app") != 0) return -1;
    if (ensure_dir("app/neural") != 0) return -1;
    if (ensure_dir("app/neural/integrations") != 0) return -1;
    if (snprintf(path, sizeof(path), "app/neural/integrations/%s_integration.c", resource) < 0) return -1;

    if (snprintf(buffer, sizeof(buffer),
                 "/* Auto-generated neural integration module: %s */\n"
                 "#include \"core/llm_integration.h\"\n\n"
                 "void %s_integration_init(LLMIntegration *llm) {\n"
                 "    if (!llm) return;\n"
                 "    llm_integration_init(llm, \"%s\");\n"
                 "}\n\n"
                 "int %s_integration_run(LLMIntegration *llm,\n"
                 "                       const char *prompt_template,\n"
                 "                       const NeuralPromptVar *vars,\n"
                 "                       int var_count,\n"
                 "                       char *out,\n"
                 "                       int out_size) {\n"
                 "    if (!llm || !prompt_template || !out || out_size <= 0) return -1;\n"
                 "    return llm_integration_run(llm, prompt_template, vars, var_count, out, out_size);\n"
                 "}\n",
                 resource,
                 resource,
                 resource,
                 resource) < 0) {
        return -1;
    }

    return write_template_file(path, buffer);
}

int forge_generate_neural_policy(const char *name) {
    char resource[64];
    char path[256];
    char buffer[3072];

    if (forge_neural_resource_name(name, resource, sizeof(resource)) != 0) return -1;
    if (ensure_dir("app") != 0) return -1;
    if (ensure_dir("app/neural") != 0) return -1;
    if (ensure_dir("app/neural/policies") != 0) return -1;
    if (snprintf(path, sizeof(path), "app/neural/policies/%s_policy.c", resource) < 0) return -1;

    if (snprintf(buffer, sizeof(buffer),
                 "/* Auto-generated neural policy module: %s */\n"
                 "#include <string.h>\n\n"
                 "int %s_policy_allow_prompt(const char *prompt) {\n"
                 "    if (!prompt) return 0;\n"
                 "    if (strstr(prompt, \"password\") != NULL) return 0;\n"
                 "    if (strstr(prompt, \"secret\") != NULL) return 0;\n"
                 "    return 1;\n"
                 "}\n\n"
                 "int %s_policy_max_tokens(void) {\n"
                 "    return 1024;\n"
                 "}\n",
                 resource,
                 resource,
                 resource) < 0) {
        return -1;
    }

    return write_template_file(path, buffer);
}

int forge_generate_stimulus_controller(const char *name) {
    char controller_name[64];
    char path[256];
    char buffer[1024];
    size_t i;
    const char *input;

    if (!name || name[0] == '\0') return -1;
    if (ensure_javascript_structure() != 0) return -1;

    input = name;
    if (strstr(name, "_controller") != NULL) {
        size_t nlen = strlen(name);
        size_t suffix_len = strlen("_controller");
        if (nlen <= suffix_len) return -1;
        nlen -= suffix_len;
        if (nlen >= sizeof(controller_name)) nlen = sizeof(controller_name) - 1;
        memcpy(controller_name, name, nlen);
        controller_name[nlen] = '\0';
        input = controller_name;
    }

    for (i = 0; i + 1 < sizeof(controller_name) && input[i]; ++i) {
        char c = input[i];
        if (!(isalnum((unsigned char)c) || c == '_')) {
            return -1;
        }
        controller_name[i] = (char)tolower((unsigned char)c);
    }
    controller_name[i] = '\0';
    if (controller_name[0] == '\0') return -1;

    if (snprintf(path, sizeof(path), "app/javascript/controllers/%s_controller.js", controller_name) < 0) {
        return -1;
    }

    if (snprintf(buffer, sizeof(buffer),
                 "import { Controller } from \"../../../js/runtime/index.js\";\n\n"
                 "export default class extends Controller {\n"
                 "  static targets = [\"form\", \"output\"];\n\n"
                 "  connect() {\n"
                 "    /* Runs when data-controller=\"%s\" is connected. */\n"
                 "  }\n\n"
                 "  submit(event) {\n"
                 "    if (event) event.preventDefault();\n"
                 "    const value = this.hasTarget(\"output\") ? this.target(\"output\").textContent : \"\";\n"
                 "    if (this.hasTarget(\"output\")) this.target(\"output\").textContent = value || \"submitted\";\n"
                 "  }\n"
                 "}\n",
                 controller_name) < 0) {
        return -1;
    }

    return write_template_file(path, buffer);
}

/* Lowercase a string into an output buffer (best-effort; does not sanitize). */
static void str_to_lower(const char *in, char *out, size_t out_size) {
    size_t i;
    if (!in || !out || out_size == 0) return;
    for (i = 0; i + 1 < out_size && in[i]; ++i) {
        out[i] = (char)tolower((unsigned char)in[i]);
    }
    out[i] = '\0';
}

static int normalize_resource_names(const char *input,
                                    char *resource_singular,
                                    size_t singular_size,
                                    char *resource_plural,
                                    size_t plural_size) {
    size_t len;
    int add_es = 0;

    if (!input || !resource_singular || !resource_plural || singular_size == 0 || plural_size == 0) {
        return -1;
    }

    str_to_lower(input, resource_singular, singular_size);
    len = strlen(resource_singular);
    if (len == 0) {
        return -1;
    }

    if (len > 4 && strcmp(resource_singular + len - 4, "sses") == 0) {
        resource_singular[len - 2] = '\0';
    } else if (len > 1 && resource_singular[len - 1] == 's' && resource_singular[len - 2] != 's') {
        resource_singular[len - 1] = '\0';
    }

    len = strlen(resource_singular);
    if (len == 0) {
        return -1;
    }

    if (len > 1 && resource_singular[len - 1] == 'y') {
        char before_y = resource_singular[len - 2];
        int vowel_before_y = (before_y == 'a' || before_y == 'e' || before_y == 'i' || before_y == 'o' || before_y == 'u');
        if (!vowel_before_y) {
            if (snprintf(resource_plural, plural_size, "%.*sies", (int)(len - 1), resource_singular) < 0) return -1;
            return 0;
        }
    }

    if (resource_singular[len - 1] == 's' ||
        resource_singular[len - 1] == 'x' ||
        resource_singular[len - 1] == 'z' ||
        (len > 1 && resource_singular[len - 2] == 'c' && resource_singular[len - 1] == 'h') ||
        (len > 1 && resource_singular[len - 2] == 's' && resource_singular[len - 1] == 'h')) {
        add_es = 1;
    }

    if (snprintf(resource_plural, plural_size, "%s%s", resource_singular, add_es ? "es" : "s") < 0) {
        return -1;
    }
    return 0;
}

/* Extract attribute key from "name:type" into out buffer. */
static int parse_attribute_key(const char *attr, char *out, size_t out_size) {
    const char *colon;
    size_t klen;
    if (!attr || !out || out_size == 0) return -1;
    colon = strchr(attr, ':');
    if (!colon) {
        if (snprintf(out, out_size, "%s", attr) < 0) return -1;
        return 0;
    }
    klen = (size_t)(colon - attr);
    if (klen >= out_size) klen = out_size - 1;
    memcpy(out, attr, klen);
    out[klen] = '\0';
    return 0;
}

static int ensure_javascript_structure(void) {
    const char *app_jsx =
        "import React from \"react\";\n"
        "import { createRoot } from \"react-dom/client\";\n"
        "import { componentRegistry } from \"./resources/index.jsx\";\n\n"
        "function parseProps(node) {\n"
        "  const raw = node.getAttribute(\"data-react-props\") || \"{}\";\n"
        "  try { return JSON.parse(raw); } catch (_err) { return {}; }\n"
        "}\n\n"
        "function mountReactIslands() {\n"
        "  const nodes = document.querySelectorAll(\"[data-react-component]\");\n"
        "  nodes.forEach((node) => {\n"
        "    const name = node.getAttribute(\"data-react-component\");\n"
        "    const Component = componentRegistry[name];\n"
        "    if (!Component) {\n"
        "      console.warn(`Cortex React: component '${name}' not found`);\n"
        "      return;\n"
        "    }\n"
        "    const props = parseProps(node);\n"
        "    const root = createRoot(node);\n"
        "    root.render(React.createElement(Component, props));\n"
        "  });\n"
        "}\n\n"
        "if (document.readyState === \"loading\") {\n"
        "  document.addEventListener(\"DOMContentLoaded\", mountReactIslands);\n"
        "} else {\n"
        "  mountReactIslands();\n"
        "}\n";
    const char *app_js =
        "import { Application } from \"../../js/runtime/index.js\";\n"
        "import { registerControllers } from \"./controllers/index.js\";\n\n"
        "const application = new Application();\n"
        "registerControllers(application);\n"
        "application.start();\n";
    const char *resources_index =
        "/* Auto-generated by Cortex scaffold. */\n"
        "export const componentRegistry = {};\n";
    const char *controllers_index =
        "/* This file is auto-generated by Cortex CLI (assets:build/dev). */\n"
        "export function registerControllers(_application) {\n"
        "  /* Controllers are registered automatically during JS build. */\n"
        "}\n";
    const char *runtime_js =
        "export class Controller {\n"
        "  constructor(context) { this.context = context; this.element = context.element; this.identifier = context.identifier; this.targetsByName = context.targetsByName; }\n"
        "  connect() {}\n"
        "  disconnect() {}\n"
        "  target(name) { const list = this.targetsByName[name] || []; return list[0] || null; }\n"
        "  targets(name) { return this.targetsByName[name] || []; }\n"
        "  hasTarget(name) { return this.targets(name).length > 0; }\n"
        "}\n"
        "export class Application {\n"
        "  constructor() { this.registry = new Map(); this.instances = []; }\n"
        "  register(identifier, ControllerClass) { this.registry.set(identifier, ControllerClass); }\n"
        "  start() { this.connectTree(document); }\n"
        "  connectTree(root) { root.querySelectorAll('[data-controller]').forEach((element) => this.connectElement(element)); }\n"
        "  connectElement(element) { const ids = (element.getAttribute('data-controller') || '').trim().split(/\\s+/).filter(Boolean); ids.forEach((identifier) => { const ControllerClass = this.registry.get(identifier); if (!ControllerClass) { console.error(`Cortex Stimulus: controller \\\"${identifier}\\\" not found`); return; } const targetsByName = this.collectTargets(identifier, element); const instance = new ControllerClass({ element, identifier, targetsByName }); this.instances.push(instance); if (typeof instance.connect === 'function') instance.connect(); this.bindActions(identifier, instance, element); }); }\n"
        "  collectTargets(identifier, element) { const map = {}; element.querySelectorAll(`[data-${identifier}-target]`).forEach((node) => { const names = (node.getAttribute(`data-${identifier}-target`) || '').split(/\\s+/).filter(Boolean); names.forEach((name) => { map[name] = map[name] || []; map[name].push(node); }); }); return map; }\n"
        "  bindActions(identifier, instance, root) { const nodes = [root, ...root.querySelectorAll('[data-action]')]; nodes.forEach((node) => { const actions = (node.getAttribute('data-action') || '').split(/\\s+/).filter(Boolean); actions.forEach((action) => { const m = action.match(/^([^->]+)->([^#]+)#(.+)$/); if (!m) { console.warn(`Cortex Stimulus: invalid action \\\"${action}\\\"`); return; } const eventName = m[1]; const actionController = m[2]; const methodName = m[3]; if (actionController !== identifier) return; if (typeof instance[methodName] !== 'function') { console.warn(`Cortex Stimulus: missing method ${identifier}#${methodName}`); return; } node.addEventListener(eventName, (event) => instance[methodName](event)); }); }); }\n"
        "}\n";

    if (ensure_dir("app") != 0) return -1;
    if (ensure_dir("app/javascript") != 0) return -1;
    if (ensure_dir("app/javascript/controllers") != 0) return -1;
    if (ensure_dir("app/javascript/resources") != 0) return -1;
    if (ensure_dir("js") != 0) return -1;
    if (ensure_dir("js/runtime") != 0) return -1;
    if (write_template_file("app/javascript/application.jsx", app_jsx) != 0) return -1;
    if (write_template_file("app/javascript/application.js", app_js) != 0) return -1;
    if (write_template_file("app/javascript/controllers/index.js", controllers_index) != 0) return -1;
    if (write_template_file("app/javascript/resources/index.jsx", resources_index) != 0) return -1;
    if (write_template_file("js/runtime/index.js", runtime_js) != 0) return -1;
    if (ensure_app_assets_at(NULL) != 0) return -1;
    return 0;
}

/* Extract attribute type from "name:type" into out buffer (lowercase). */
static int parse_attribute_type(const char *attr, char *out, size_t out_size) {
    const char *colon;
    size_t i = 0;
    if (!out || out_size == 0) return -1;
    out[0] = '\0';
    if (!attr) return -1;
    colon = strchr(attr, ':');
    if (!colon || colon[1] == '\0') return 0;
    ++colon;
    while (colon[i] && i + 1 < out_size) {
        out[i] = (char)tolower((unsigned char)colon[i]);
        ++i;
    }
    out[i] = '\0';
    return 0;
}

/* Append one scaffold form field to view_buf based on attribute type. */
static int append_form_field(char *view_buf,
                             size_t view_buf_size,
                             size_t *view_len,
                             const char *key,
                             const char *field,
                             const char *type) {
    size_t appended;
    if (!view_buf || !view_len || !key || !field || !type) return -1;

    if (strcmp(type, "text") == 0) {
        appended = (size_t)snprintf(view_buf + *view_len, view_buf_size - *view_len,
                                    "  <div>\n"
                                    "    <label for=\"%s\">%s</label>\n"
                                    "    <textarea id=\"%s\" name=\"%s\"></textarea>\n"
                                    "  </div>\n",
                                    field, key, field, field);
    } else if (strcmp(type, "boolean") == 0 || strcmp(type, "bool") == 0) {
        appended = (size_t)snprintf(view_buf + *view_len, view_buf_size - *view_len,
                                    "  <div>\n"
                                    "    <label for=\"%s\">%s</label>\n"
                                    "    <input type=\"checkbox\" id=\"%s\" name=\"%s\" />\n"
                                    "  </div>\n",
                                    field, key, field, field);
    } else if (strcmp(type, "email") == 0 || strcmp(field, "email") == 0) {
        appended = (size_t)snprintf(view_buf + *view_len, view_buf_size - *view_len,
                                    "  <div>\n"
                                    "    <label for=\"%s\">%s</label>\n"
                                    "    <input type=\"email\" id=\"%s\" name=\"%s\" />\n"
                                    "  </div>\n",
                                    field, key, field, field);
    } else {
        appended = (size_t)snprintf(view_buf + *view_len, view_buf_size - *view_len,
                                    "  <div>\n"
                                    "    <label for=\"%s\">%s</label>\n"
                                    "    <input type=\"text\" id=\"%s\" name=\"%s\" />\n"
                                    "  </div>\n",
                                    field, key, field, field);
    }

    *view_len += appended;
    if (*view_len >= view_buf_size) return -1;
    return 0;
}

static int write_scaffold_react_resource_files(const char *resource,
                                               const char *resource_plural,
                                               int attr_count,
                                               const char **attributes) {
    char resources_dir[256];
    char resource_dir[256];
    char resource_file[512];
    char registry_file[256];
    ForgeStrBuf js;
    int i;

    if (!resource || !resource_plural || attr_count < 0) return -1;

    if (snprintf(resources_dir, sizeof(resources_dir), "app/javascript/resources") < 0) return -1;
    if (snprintf(resource_dir, sizeof(resource_dir), "app/javascript/resources/%s", resource_plural) < 0) return -1;
    if (snprintf(resource_file, sizeof(resource_file), "%s/index.jsx", resource_dir) < 0) return -1;
    if (snprintf(registry_file, sizeof(registry_file), "app/javascript/resources/index.jsx") < 0) return -1;

    if (ensure_dir("app") != 0) return -1;
    if (ensure_dir("app/javascript") != 0) return -1;
    if (ensure_dir(resources_dir) != 0) return -1;
    if (ensure_dir(resource_dir) != 0) return -1;

    js.p = NULL;
    js.len = 0;
    js.cap = 0;

    if (forge_str_fmt(&js,
                      "import React, { useEffect, useMemo, useState } from \"react\";\n\n"
                      "const resource = \"%s\";\n"
                      "const fields = [\n",
                      resource_plural) != 0) {
        forge_str_free(&js);
        return -1;
    }

    for (i = 0; i < attr_count; ++i) {
        char key[64];
        char field[64];
        char type[32];
        if (parse_attribute_key(attributes[i], key, sizeof(key)) != 0) {
            forge_str_free(&js);
            return -1;
        }
        str_to_lower(key, field, sizeof(field));
        if (parse_attribute_type(attributes[i], type, sizeof(type)) != 0) {
            forge_str_free(&js);
            return -1;
        }
        if (forge_str_fmt(&js,
                          "  { name: \"%s\", label: \"%s\", type: \"%s\" },\n",
                          field,
                          key,
                          type[0] ? type : "string") != 0) {
            forge_str_free(&js);
            return -1;
        }
    }
    if (forge_str_fmt(
            &js,
            "];\n\n"
            "function coerceValue(field, raw) {\n"
            "  if (field.type === \"boolean\" || field.type === \"bool\") return !!raw;\n"
            "  return raw == null ? \"\" : String(raw);\n"
            "}\n\n"
            "function defaultFormState() {\n"
            "  const next = {};\n"
            "  fields.forEach((field) => {\n"
            "    next[field.name] = field.type === \"boolean\" || field.type === \"bool\" ? false : \"\";\n"
            "  });\n"
            "  return next;\n"
            "}\n\n"
            "export function %sIndexPage(props) {\n"
            "  const [rows, setRows] = useState([]);\n"
            "  const [loading, setLoading] = useState(true);\n"
            "  const endpoint = props.indexJsonPath || `/${resource}.json`;\n"
            "  const deleteRecord = (id) => {\n"
            "    if (!window.confirm(\"Delete this record?\")) return;\n"
            "    fetch(`/${resource}/${id}`, {\n"
            "      method: \"POST\",\n"
            "      headers: { \"Content-Type\": \"application/x-www-form-urlencoded\" },\n"
            "      body: \"_method=DELETE\",\n"
            "    }).then((res) => {\n"
            "      if (res.ok) {\n"
            "        setRows((prev) => prev.filter((row) => String(row.id) !== String(id)));\n"
            "      }\n"
            "    });\n"
            "  };\n"
            "  useEffect(() => {\n"
            "    let active = true;\n"
            "    fetch(endpoint)\n"
            "      .then((res) => res.json())\n"
            "      .then((json) => { if (active) setRows(Array.isArray(json.data) ? json.data : []); })\n"
            "      .catch(() => { if (active) setRows([]); })\n"
            "      .finally(() => { if (active) setLoading(false); });\n"
            "    return () => { active = false; };\n"
            "  }, [endpoint]);\n"
            "  if (loading) return React.createElement(\"p\", null, \"Loading...\");\n"
            "  return React.createElement(\"div\", null,\n"
            "    React.createElement(\"h1\", null, props.title || \"%s\"),\n"
            "    React.createElement(\"p\", null, React.createElement(\"a\", { href: props.newPath || `/${resource}/new` }, \"New %s\")),\n"
            "    React.createElement(\"table\", { border: \"1\", cellPadding: \"6\", cellSpacing: \"0\" },\n"
            "      React.createElement(\"thead\", null,\n"
            "        React.createElement(\"tr\", null,\n"
            "          React.createElement(\"th\", null, \"id\"),\n"
            "          ...fields.map((field) => React.createElement(\"th\", { key: field.name }, field.label)),\n"
            "          React.createElement(\"th\", null, \"Actions\")\n"
            "        )\n"
            "      ),\n"
            "      React.createElement(\"tbody\", null,\n"
            "        rows.length === 0\n"
            "          ? React.createElement(\"tr\", null, React.createElement(\"td\", { colSpan: String(fields.length + 2) }, \"No records yet.\"))\n"
            "          : rows.map((row) => React.createElement(\"tr\", { key: row.id },\n"
            "              React.createElement(\"td\", null, React.createElement(\"a\", { href: `/${resource}/${row.id}` }, row.id)),\n"
            "              ...fields.map((field) => React.createElement(\"td\", { key: field.name }, String(row[field.name] ?? \"\"))),\n"
            "              React.createElement(\"td\", null,\n"
            "                React.createElement(\"a\", { href: `/${resource}/${row.id}` }, \"Show\"),\n"
            "                \" | \",\n"
            "                React.createElement(\"a\", { href: `/${resource}/${row.id}/edit` }, \"Edit\"),\n"
            "                \" | \",\n"
            "                React.createElement(\"button\", { type: \"button\", onClick: () => deleteRecord(row.id) }, \"Delete\")\n"
            "              )\n"
            "            ))\n"
            "      )\n"
            "    )\n"
            "  );\n"
            "}\n\n"
            "export function %sShowPage(props) {\n"
            "  const [row, setRow] = useState(null);\n"
            "  const endpoint = useMemo(() => {\n"
            "    const p = window.location.pathname.replace(/\\/$/, \"\");\n"
            "    return p.endsWith(\".json\") ? p : `${p}.json`;\n"
            "  }, []);\n"
            "  const deleteCurrent = () => {\n"
            "    if (!row || !window.confirm(\"Delete this record?\")) return;\n"
            "    fetch(`/${resource}/${row.id}`, {\n"
            "      method: \"POST\",\n"
            "      headers: { \"Content-Type\": \"application/x-www-form-urlencoded\" },\n"
            "      body: \"_method=DELETE\",\n"
            "    }).then((res) => {\n"
            "      if (res.ok) window.location.assign(`/${resource}`);\n"
            "    });\n"
            "  };\n"
            "  useEffect(() => {\n"
            "    let active = true;\n"
            "    fetch(endpoint)\n"
            "      .then((res) => res.json())\n"
            "      .then((json) => { if (active) setRow(json.data || null); })\n"
            "      .catch(() => { if (active) setRow(null); });\n"
            "    return () => { active = false; };\n"
            "  }, [endpoint]);\n"
            "  if (!row) return React.createElement(\"p\", null, \"Record not found.\");\n"
            "  return React.createElement(\"div\", null,\n"
            "    React.createElement(\"h1\", null, props.title || \"%s\"),\n"
            "    React.createElement(\"dl\", null,\n"
            "      React.createElement(\"dt\", null, \"id\"),\n"
            "      React.createElement(\"dd\", null, String(row.id)),\n"
            "      ...fields.flatMap((field) => [\n"
            "        React.createElement(\"dt\", { key: `${field.name}-dt` }, field.label),\n"
            "        React.createElement(\"dd\", { key: `${field.name}-dd` }, String(row[field.name] ?? \"\"))\n"
            "      ])\n"
            "    ),\n"
            "    React.createElement(\"p\", null,\n"
            "      React.createElement(\"a\", { href: `/${resource}/${row.id}/edit` }, \"Edit\"),\n"
            "      \" | \",\n"
            "      React.createElement(\"button\", { type: \"button\", onClick: deleteCurrent }, \"Delete\")\n"
            "    ),\n"
            "    React.createElement(\"p\", null, React.createElement(\"a\", { href: `/${resource}` }, \"Back to list\"))\n"
            "  );\n"
            "}\n\n"
            "export function %sFormPage(props) {\n"
            "  const [values, setValues] = useState(defaultFormState());\n"
            "  const isEdit = props.mode === \"edit\";\n"
            "  const path = window.location.pathname.replace(/\\/$/, \"\");\n"
            "  const match = path.match(new RegExp(`/${resource}/(\\\\d+)(/edit)?$`));\n"
            "  const recordId = match ? match[1] : null;\n"
            "  const action = isEdit && recordId ? `/${resource}/${recordId}` : `/${resource}`;\n"
            "  useEffect(() => {\n"
            "    if (!isEdit || !recordId) return;\n"
            "    fetch(`/${resource}/${recordId}.json`)\n"
            "      .then((res) => res.json())\n"
            "      .then((json) => {\n"
            "        const row = json.data || {};\n"
            "        const next = defaultFormState();\n"
            "        fields.forEach((field) => { next[field.name] = coerceValue(field, row[field.name]); });\n"
            "        setValues(next);\n"
            "      })\n"
            "      .catch(() => {});\n"
            "  }, [isEdit, recordId]);\n"
            "  const setField = (name, value) => setValues((prev) => ({ ...prev, [name]: value }));\n"
            "  return React.createElement(\"div\", null,\n"
            "    React.createElement(\"h1\", null, props.title || (isEdit ? \"Edit\" : \"New\")),\n"
            "    React.createElement(\"form\", { method: \"POST\", action },\n"
            "      isEdit ? React.createElement(\"input\", { type: \"hidden\", name: \"_method\", value: \"PUT\" }) : null,\n"
            "      ...fields.map((field) => React.createElement(\"div\", { key: field.name },\n"
            "        React.createElement(\"label\", { htmlFor: field.name }, field.label),\n"
            "        field.type === \"text\"\n"
            "          ? React.createElement(\"textarea\", { id: field.name, name: field.name, value: values[field.name], onChange: (e) => setField(field.name, e.target.value) })\n"
            "          : field.type === \"boolean\" || field.type === \"bool\"\n"
            "          ? React.createElement(\"input\", { type: \"checkbox\", id: field.name, name: field.name, checked: !!values[field.name], onChange: (e) => setField(field.name, e.target.checked) })\n"
            "          : React.createElement(\"input\", { type: field.type === \"email\" ? \"email\" : \"text\", id: field.name, name: field.name, value: values[field.name], onChange: (e) => setField(field.name, e.target.value) })\n"
            "      )),\n"
            "      React.createElement(\"button\", { type: \"submit\" }, isEdit ? \"Update\" : \"Create\")\n"
            "    )\n"
            "  );\n"
            "}\n\n"
            "export default {\n"
            "  %sIndexPage,\n"
            "  %sShowPage,\n"
            "  %sFormPage,\n"
            "};\n",
            resource_plural,
            resource_plural,
            resource,
            resource_plural,
            resource_plural,
            resource_plural,
            resource_plural,
            resource_plural,
            resource_plural,
            resource_plural) != 0) {
        forge_str_free(&js);
        return -1;
    }

    if (!js.p || write_template_file(resource_file, js.p) != 0) {
        forge_str_free(&js);
        return -1;
    }
    forge_str_free(&js);

    {
        char registry_buf[2048];
        if (snprintf(registry_buf, sizeof(registry_buf),
                     "import pages from \"./%s/index.jsx\";\n\n"
                     "export const componentRegistry = {\n"
                     "  %sIndexPage: pages.%sIndexPage,\n"
                     "  %sShowPage: pages.%sShowPage,\n"
                     "  %sFormPage: pages.%sFormPage,\n"
                     "};\n",
                     resource_plural,
                     resource_plural,
                     resource_plural,
                     resource_plural,
                     resource_plural,
                     resource_plural,
                     resource_plural) < 0) {
            return -1;
        }
        if (write_template_file(registry_file, registry_buf) != 0) return -1;
    }
    return 0;
}

static const char *attr_sql_type(const char *lowercase_type) {
    if (!lowercase_type || !lowercase_type[0]) {
        return "TEXT";
    }
    if (strcmp(lowercase_type, "integer") == 0 || strcmp(lowercase_type, "int") == 0) {
        return "INTEGER";
    }
    if (strcmp(lowercase_type, "float") == 0 || strcmp(lowercase_type, "decimal") == 0 ||
        strcmp(lowercase_type, "real") == 0) {
        return "REAL";
    }
    if (strcmp(lowercase_type, "boolean") == 0 || strcmp(lowercase_type, "bool") == 0) {
        return "INTEGER";
    }
    return "TEXT";
}

static int write_default_application_layout_if_missing(void) {
    static const char *path = "app/views/layouts/application.html";
    FILE *f;
    static const char *content =
        "<!DOCTYPE html>\n"
        "<html lang=\"en\">\n"
        "<head>\n"
        "  <meta charset=\"UTF-8\" />\n"
        "  <title>Cortex App</title>\n"
        "  <link rel=\"stylesheet\" href=\"/assets/stylesheets/application.css\" />\n"
        "</head>\n"
        "<body>\n"
        "{{yield}}\n"
        "</body>\n"
        "</html>\n";

    f = fopen(path, "r");
    if (f) {
        fclose(f);
        return 0;
    }
    printf("forge_scaffold: generating layout at '%s'\n", path);
    return write_template_file(path, content);
}

static int write_scaffold_sql_migration(const char *table_name, int attr_count, const char **attributes) {
    char ts[32];
    char path[512];
    char sql_buf[8192];
    size_t off;
    time_t now;
    struct tm *tm_info;
    int i;

    if (!table_name || !table_name[0]) {
        return -1;
    }
    now = time(NULL);
    tm_info = localtime(&now);
    if (!tm_info) {
        return -1;
    }
    if (strftime(ts, sizeof(ts), "%Y%m%d%H%M%S", tm_info) == 0) {
        return -1;
    }
    if (snprintf(path, sizeof(path), "db/migrate/%s_create_%s.sql", ts, table_name) < 0) {
        return -1;
    }

    off = (size_t)snprintf(sql_buf, sizeof(sql_buf),
                           "CREATE TABLE %s (\n"
                           "  id INTEGER PRIMARY KEY AUTOINCREMENT",
                           table_name);
    if (off >= sizeof(sql_buf)) {
        return -1;
    }

    for (i = 0; i < attr_count; ++i) {
        char key[64];
        char field[64];
        char type[32];
        const char *sqlt;
        if (!attributes || !attributes[i]) {
            return -1;
        }
        if (parse_attribute_key(attributes[i], key, sizeof(key)) != 0) {
            return -1;
        }
        str_to_lower(key, field, sizeof(field));
        if (parse_attribute_type(attributes[i], type, sizeof(type)) != 0) {
            return -1;
        }
        sqlt = attr_sql_type(type);
        off += (size_t)snprintf(sql_buf + off, sizeof(sql_buf) - off, ",\n  %s %s", field, sqlt);
        if (off >= sizeof(sql_buf)) {
            return -1;
        }
    }

    off += (size_t)snprintf(sql_buf + off, sizeof(sql_buf) - off,
                             ",\n  created_at DATETIME,\n"
                             "  updated_at DATETIME\n"
                             ");\n");
    if (off >= sizeof(sql_buf)) {
        return -1;
    }

    printf("forge_scaffold: generating SQL migration at '%s'\n", path);
    return write_template_file(path, sql_buf);
}

static int write_scaffold_neural_template(const char *resource, const char *resource_plural) {
    char path[256];
    char neural_buf[2048];
    size_t len;

    if (!resource || !resource[0] || !resource_plural || !resource_plural[0]) {
        return -1;
    }
    if (snprintf(path, sizeof(path), "app/neural/%s_neural_model.c", resource) < 0) {
        return -1;
    }

    len = (size_t)snprintf(neural_buf, sizeof(neural_buf),
                           "/* Auto-generated scaffold neural entry: %s */\n"
                           "#include \"core/neural_model.h\"\n\n"
                           "/*\n"
                           " * Starting point for AI features connected to %s.\n"
                           " *\n"
                           " * Suggested next steps:\n"
                           " * 1) Define the system prompt with domain rules.\n"
                           " * 2) Build context from your local data before each call.\n"
                           " * 3) Map AI outputs back to deterministic app actions.\n"
                           " */\n"
                           "static const char *%s_system_prompt =\n"
                           "    \"You are an assistant for %s records. \"\n"
                           "    \"Answer with concise language and include key business rules.\";\n\n"
                           "const char *%s_neural_system_prompt(void) {\n"
                           "    return %s_system_prompt;\n"
                           "}\n\n"
                           "void %s_neural_enrich(NeuralModel *model) {\n"
                           "    (void)model;\n"
                           "    /* TODO: configure provider/model settings for %s features. */\n"
                           "    /* TODO: enrich prompt context with relevant %s data. */\n"
                           "}\n",
                           resource,
                           resource_plural,
                           resource,
                           resource_plural,
                           resource,
                           resource,
                           resource,
                           resource_plural,
                           resource_plural);
    if (len >= sizeof(neural_buf)) {
        return -1;
    }

    printf("forge_scaffold: generating neural file at '%s'\n", path);
    return write_template_file(path, neural_buf);
}

/* Generate <resource> scaffold with model/controller/views/routes files. */
int forge_generate_scaffold(const char *resource_name, int attr_count, const char **attributes, int use_react) {
    char resource[64];
    char resource_plural[64];
    char type_name[64];
    char macro_name[64];
    char path[256];
    char views_dir[256];
    char model_buf[4096];
    char routes_buf[4096];
    ForgeStrBuf scaffold_controller_buf;
    int i;
    size_t len = 0;

    if (!resource_name || resource_name[0] == '\0') return -1;
    if (attr_count < 0) return -1;
    if (attr_count > 0 && !attributes) return -1;

    if (normalize_resource_names(resource_name, resource, sizeof(resource), resource_plural, sizeof(resource_plural)) != 0) return -1;
    model_type_name(resource, type_name, sizeof(type_name));
    model_macro_name(resource, macro_name, sizeof(macro_name));

    if (ensure_dir("app") != 0) return -1;
    if (ensure_dir("app/models") != 0) return -1;
    if (ensure_dir("app/neural") != 0) return -1;
    if (ensure_dir("app/controllers") != 0) return -1;
    if (ensure_dir("app/views") != 0) return -1;
    if (ensure_dir("app/views/layouts") != 0) return -1;
    if (snprintf(views_dir, sizeof(views_dir), "app/views/%s", resource_plural) < 0) return -1;
    if (ensure_dir(views_dir) != 0) return -1;
    if (ensure_dir("db") != 0) return -1;
    if (ensure_dir("db/migrate") != 0) return -1;
    if (ensure_dir("config") != 0) return -1;
    if (ensure_javascript_structure() != 0) return -1;

    if (write_scaffold_sql_migration(resource_plural, attr_count, attributes) != 0) {
        perror("forge_scaffold sql migration");
        return -1;
    }
    if (write_default_application_layout_if_missing() != 0) {
        perror("forge_scaffold layout");
        return -1;
    }

    /* model */
    if (snprintf(path, sizeof(path), "app/models/%s.c", resource) < 0) return -1;
    len = (size_t)snprintf(model_buf, sizeof(model_buf),
                           "/* Auto-generated scaffold model: %s */\n"
                           "#include \"core/active_record.h\"\n"
                           "#include \"core/active_model.h\"\n\n"
                           "#define %s_MODEL_NAME \"%s\"\n\n"
                           "typedef ActiveModel %s;\n\n"
                           "%s *%s_create(ActiveRecordStore *store) {\n"
                           "    return active_record_create(store, %s_MODEL_NAME);\n"
                           "}\n\n",
                           resource, macro_name, resource, type_name, type_name, resource, macro_name);
    if (len >= sizeof(model_buf)) return -1;
    for (i = 0; i < attr_count; ++i) {
        const char *attr = attributes[i];
        char key[64];
        char field[64];
        if (!attr) return -1;
        if (parse_attribute_key(attr, key, sizeof(key)) != 0) return -1;
        str_to_lower(key, field, sizeof(field));
        len += (size_t)snprintf(model_buf + len, sizeof(model_buf) - len,
                                "int %s_set_%s(%s *m, const char *%s) {\n"
                                "    return active_model_set_field(m, \"%s\", %s);\n"
                                "}\n\n",
                                resource, field, type_name, field, field, field);
        if (len >= sizeof(model_buf)) return -1;
    }
    printf("forge_scaffold: generating model at '%s'\n", path);
    if (write_template_file(path, model_buf) != 0) { perror("forge_scaffold model"); return -1; }
    if (write_scaffold_neural_template(resource, resource_plural) != 0) {
        perror("forge_scaffold neural");
        return -1;
    }

    /* controller (index/show query SQLite; render_html + layout) */
    if (snprintf(path, sizeof(path), "app/controllers/%s_controller.c", resource_plural) < 0) return -1;
    scaffold_controller_buf.p = NULL;
    scaffold_controller_buf.len = 0;
    scaffold_controller_buf.cap = 0;
    if ((use_react
             ? forge_emit_scaffold_controller_react(&scaffold_controller_buf, resource_plural, attr_count, attributes)
             : forge_emit_scaffold_controller(&scaffold_controller_buf, resource, resource_plural, type_name, attr_count, attributes)) != 0) {
        forge_str_free(&scaffold_controller_buf);
        return -1;
    }
    printf("forge_scaffold: generating controller at '%s'\n", path);
    if (!scaffold_controller_buf.p || write_template_file(path, scaffold_controller_buf.p) != 0) {
        forge_str_free(&scaffold_controller_buf);
        perror("forge_scaffold controller");
        return -1;
    }
    forge_str_free(&scaffold_controller_buf);

    /* views */
    {
        char view_buf[4096];
        size_t view_len = 0;
        if (use_react) {
            if (snprintf(path, sizeof(path), "%s/index.html", views_dir) < 0) return -1;
            if (snprintf(view_buf, sizeof(view_buf),
                         "<div data-react-component=\"%sIndexPage\" data-react-props='{\"title\":\"%s\",\"newPath\":\"/%s/new\",\"indexJsonPath\":\"/%s.json\"}'>\n"
                         "  <h1>%s</h1>\n"
                         "  <p><a href=\"/%s/new\">New %s</a></p>\n"
                         "  <p>Loading...</p>\n"
                         "</div>\n",
                         resource_plural, type_name, resource_plural, resource_plural,
                         type_name, resource_plural, type_name) < 0) return -1;
            if (write_template_file(path, view_buf) != 0) { perror("forge_scaffold react index"); return -1; }

            if (snprintf(path, sizeof(path), "%s/show.html", views_dir) < 0) return -1;
            if (snprintf(view_buf, sizeof(view_buf),
                         "<div data-react-component=\"%sShowPage\" data-react-props='{\"title\":\"%s\"}'>\n"
                         "  <h1>%s</h1>\n"
                         "  <p>Loading...</p>\n"
                         "  <p><a href=\"/%s\">Back to list</a></p>\n"
                         "</div>\n",
                         resource_plural, type_name, type_name, resource_plural) < 0) return -1;
            if (write_template_file(path, view_buf) != 0) { perror("forge_scaffold react show"); return -1; }

            if (snprintf(path, sizeof(path), "%s/new.html", views_dir) < 0) return -1;
            view_len = (size_t)snprintf(view_buf, sizeof(view_buf),
                                        "<div data-react-component=\"%sFormPage\" data-react-props='{\"title\":\"New %s\",\"mode\":\"new\"}'>\n"
                                        "<h1>New %s</h1>\n"
                                        "<form method=\"POST\" action=\"/%s\">\n",
                                        resource_plural, type_name, type_name, resource_plural);
            if (view_len >= sizeof(view_buf)) return -1;
            for (i = 0; i < attr_count; ++i) {
                const char *attr = attributes[i];
                char key[64];
                char field[64];
                char type[32];
                if (!attr) return -1;
                if (parse_attribute_key(attr, key, sizeof(key)) != 0) return -1;
                str_to_lower(key, field, sizeof(field));
                if (parse_attribute_type(attr, type, sizeof(type)) != 0) return -1;
                if (append_form_field(view_buf, sizeof(view_buf), &view_len, key, field, type) != 0) return -1;
            }
            view_len += (size_t)snprintf(view_buf + view_len, sizeof(view_buf) - view_len,
                                         "  <button type=\"submit\">Create</button>\n"
                                         "</form>\n"
                                         "</div>\n");
            if (view_len >= sizeof(view_buf)) return -1;
            if (write_template_file(path, view_buf) != 0) { perror("forge_scaffold react new"); return -1; }

            if (snprintf(path, sizeof(path), "%s/edit.html", views_dir) < 0) return -1;
            view_len = (size_t)snprintf(view_buf, sizeof(view_buf),
                                        "<div data-react-component=\"%sFormPage\" data-react-props='{\"title\":\"Edit %s\",\"mode\":\"edit\"}'>\n"
                                        "<h1>Edit %s</h1>\n"
                                        "<form method=\"POST\" action=\"/%s/1\">\n",
                                        resource_plural, type_name, type_name, resource_plural);
            if (view_len >= sizeof(view_buf)) return -1;
            for (i = 0; i < attr_count; ++i) {
                const char *attr = attributes[i];
                char key[64];
                char field[64];
                char type[32];
                if (!attr) return -1;
                if (parse_attribute_key(attr, key, sizeof(key)) != 0) return -1;
                str_to_lower(key, field, sizeof(field));
                if (parse_attribute_type(attr, type, sizeof(type)) != 0) return -1;
                if (append_form_field(view_buf, sizeof(view_buf), &view_len, key, field, type) != 0) return -1;
            }
            view_len += (size_t)snprintf(view_buf + view_len, sizeof(view_buf) - view_len,
                                         "  <input type=\"hidden\" name=\"_method\" value=\"PUT\" />\n"
                                         "  <button type=\"submit\">Update</button>\n"
                                         "</form>\n"
                                         "</div>\n");
            if (view_len >= sizeof(view_buf)) return -1;
            if (write_template_file(path, view_buf) != 0) { perror("forge_scaffold react edit"); return -1; }
        } else {
            if (snprintf(path, sizeof(path), "%s/index.html", views_dir) < 0) return -1;
            if (write_template_file(path,
                                    "<!-- Listing HTML is built in app/controllers/ — see *_index() (SQLite). -->\n") != 0) {
                perror("forge_scaffold index");
                return -1;
            }

            if (snprintf(path, sizeof(path), "%s/show.html", views_dir) < 0) return -1;
            if (write_template_file(path,
                                    "<!-- Show HTML is built in app/controllers/ — see *_show() (SQLite). -->\n") != 0) {
                perror("forge_scaffold show");
                return -1;
            }

            if (snprintf(path, sizeof(path), "%s/new.html", views_dir) < 0) return -1;
            view_len = (size_t)snprintf(view_buf, sizeof(view_buf),
                                        "<h1>New %s</h1>\n"
                                        "<form method=\"POST\" action=\"/%s\" data-controller=\"%s\" data-%s-target=\"form\" data-action=\"submit->%s#submit\">\n",
                                        resource, resource_plural, resource, resource, resource);
            if (view_len >= sizeof(view_buf)) return -1;
            for (i = 0; i < attr_count; ++i) {
                const char *attr = attributes[i];
                char key[64];
                char field[64];
                char type[32];
                if (!attr) return -1;
                if (parse_attribute_key(attr, key, sizeof(key)) != 0) return -1;
                str_to_lower(key, field, sizeof(field));
                if (parse_attribute_type(attr, type, sizeof(type)) != 0) return -1;
                if (append_form_field(view_buf, sizeof(view_buf), &view_len, key, field, type) != 0) return -1;
            }
            view_len += (size_t)snprintf(view_buf + view_len, sizeof(view_buf) - view_len,
                                         "  <button type=\"submit\">Create</button>\n"
                                         "</form>\n");
            if (view_len >= sizeof(view_buf)) return -1;
            if (write_template_file(path, view_buf) != 0) { perror("forge_scaffold new"); return -1; }

            if (snprintf(path, sizeof(path), "%s/edit.html", views_dir) < 0) return -1;
            view_len = (size_t)snprintf(view_buf, sizeof(view_buf),
                                        "<h1>Edit %s</h1>\n"
                                        "<form method=\"POST\" action=\"/%s/1\" data-controller=\"%s\" data-%s-target=\"form\" data-action=\"submit->%s#submit\">\n",
                                        resource, resource_plural, resource, resource, resource);
            if (view_len >= sizeof(view_buf)) return -1;
            for (i = 0; i < attr_count; ++i) {
                const char *attr = attributes[i];
                char key[64];
                char field[64];
                char type[32];
                if (!attr) return -1;
                if (parse_attribute_key(attr, key, sizeof(key)) != 0) return -1;
                str_to_lower(key, field, sizeof(field));
                if (parse_attribute_type(attr, type, sizeof(type)) != 0) return -1;
                if (append_form_field(view_buf, sizeof(view_buf), &view_len, key, field, type) != 0) return -1;
            }
            view_len += (size_t)snprintf(view_buf + view_len, sizeof(view_buf) - view_len,
                                         "  <input type=\"hidden\" name=\"_method\" value=\"PUT\" />\n"
                                         "  <button type=\"submit\">Update</button>\n"
                                         "</form>\n");
            if (view_len >= sizeof(view_buf)) return -1;
            if (write_template_file(path, view_buf) != 0) { perror("forge_scaffold edit"); return -1; }
        }
    }

    /* routes */
    if (snprintf(path, sizeof(path), "config/routes.c") < 0) return -1;
    if (snprintf(routes_buf, sizeof(routes_buf),
                 "#include \"config/routes.h\"\n"
                 "#include \"../action/action_request.h\"\n"
                 "#include \"../action/action_response.h\"\n"
                 "#include \"../action/action_controller.h\"\n\n"
                 "int route_get(ActionRouter *router, const char *path, ActionHandler handler) {\n"
                 "    return action_router_add_route(router, \"GET\", path, handler);\n"
                 "}\n\n"
                 "int route_post(ActionRouter *router, const char *path, ActionHandler handler) {\n"
                 "    return action_router_add_route(router, \"POST\", path, handler);\n"
                 "}\n\n"
                 "int route_put(ActionRouter *router, const char *path, ActionHandler handler) {\n"
                 "    return action_router_add_route(router, \"PUT\", path, handler);\n"
                 "}\n\n"
                 "int route_delete(ActionRouter *router, const char *path, ActionHandler handler) {\n"
                 "    return action_router_add_route(router, \"DELETE\", path, handler);\n"
                 "}\n\n"
                 "__attribute__((weak)) void %s_index(ActionRequest *req, ActionResponse *res) { (void)req; action_controller_render_text(res, 500, \"Missing controller handler: %s_index\"); }\n"
                 "__attribute__((weak)) void %s_show(ActionRequest *req, ActionResponse *res) { (void)req; action_controller_render_text(res, 500, \"Missing controller handler: %s_show\"); }\n"
                 "__attribute__((weak)) void %s_new(ActionRequest *req, ActionResponse *res) { (void)req; action_controller_render_text(res, 500, \"Missing controller handler: %s_new\"); }\n"
                 "__attribute__((weak)) void %s_edit(ActionRequest *req, ActionResponse *res) { (void)req; action_controller_render_text(res, 500, \"Missing controller handler: %s_edit\"); }\n"
                 "__attribute__((weak)) void %s_create(ActionRequest *req, ActionResponse *res) { (void)req; action_controller_render_text(res, 500, \"Missing controller handler: %s_create\"); }\n"
                 "__attribute__((weak)) void %s_update(ActionRequest *req, ActionResponse *res) { (void)req; action_controller_render_text(res, 500, \"Missing controller handler: %s_update\"); }\n"
                 "__attribute__((weak)) void %s_delete(ActionRequest *req, ActionResponse *res) { (void)req; action_controller_render_text(res, 500, \"Missing controller handler: %s_delete\"); }\n"
                 "void home_index(ActionRequest *req, ActionResponse *res);\n\n"
                 "void app_register_routes(ActionRouter *router) {\n"
                 "    if (!router) return;\n"
                 "    route_get(router, \"/\", home_index);\n"
                 "    route_get(router, \"/%s\", %s_index);\n"
                 "    route_get(router, \"/%s.json\", %s_index);\n"
                 "    route_get(router, \"/%s/new\", %s_new);\n"
                 "    route_get(router, \"/%s/:id/edit\", %s_edit);\n"
                 "    route_get(router, \"/%s/:id\", %s_show);\n"
                 "    route_get(router, \"/%s/:id.json\", %s_show);\n"
                 "    route_post(router, \"/%s\", %s_create);\n"
                 "    route_post(router, \"/%s/:id\", %s_update);\n"
                 "    route_put(router, \"/%s/:id\", %s_update);\n"
                 "    route_delete(router, \"/%s/:id\", %s_delete);\n"
                 "}\n\n"
                 "void register_routes(ActionRouter *router) {\n"
                 "    app_register_routes(router);\n"
                 "}\n",
                 resource_plural, resource_plural,
                 resource_plural, resource_plural,
                 resource_plural, resource_plural,
                 resource_plural, resource_plural,
                 resource_plural, resource_plural,
                 resource_plural, resource_plural,
                 resource_plural, resource_plural,
                 resource_plural, resource_plural,
                 resource_plural, resource_plural,
                 resource_plural, resource_plural,
                 resource_plural, resource_plural,
                 resource_plural, resource_plural,
                 resource_plural, resource_plural,
                 resource_plural, resource_plural,
                 resource_plural, resource_plural,
                 resource_plural, resource_plural,
                 resource_plural, resource_plural,
                 resource_plural, resource_plural) < 0) return -1;
    printf("forge_scaffold: updating routes in '%s'\n", path);
    if (write_template_file(path, routes_buf) != 0) { perror("forge_scaffold routes"); return -1; }

    if (use_react) {
        if (write_scaffold_react_resource_files(resource, resource_plural, attr_count, attributes) != 0) return -1;
    } else {
        if (forge_generate_stimulus_controller(resource) != 0) return -1;
    }
    return 0;
}

int forge_new_project(const char *project_name) {
    char path[320];
    char package_json_content[2048];
    const char *main_content =
        "/* Cortex app server entrypoint */\n"
        "#include <stdio.h>\n"
        "#include \"action/action_dispatch.h\"\n"
        "#include \"action/action_router.h\"\n"
        "#include \"config/routes.h\"\n"
        "#include \"db/db_bootstrap.h\"\n\n"
        "void app_register_routes(ActionRouter *router);\n\n"
        "int main(int argc, char **argv) {\n"
        "    (void)argc;\n"
        "    (void)argv;\n"
        "    if (cortex_db_bootstrap() != 0) {\n"
        "        fprintf(stderr, \"database bootstrap failed (check migration status)\\n\");\n"
        "        return 1;\n"
        "    }\n"
        "    ActionRouter router;\n"
        "    action_router_init(&router);\n"
        "    app_register_routes(&router);\n"
        "    printf(\"Listening on http://localhost:3000\\n\");\n"
        "    {\n"
        "        int rc = action_dispatch_serve_http(&router);\n"
        "        cortex_db_shutdown();\n"
        "        return rc;\n"
        "    }\n"
        "}\n";
    const char *makefile_content =
        "CC := gcc\n"
        "CORTEX_ROOT ?= ..\n"
        "CFLAGS := -Wall -Wextra -std=c11 -I. -I$(CORTEX_ROOT) -I$(CORTEX_ROOT)/core -I$(CORTEX_ROOT)/action -I$(CORTEX_ROOT)/config\n"
        "APP_SRCS := main.c config/routes.c $(wildcard app/controllers/*.c) $(wildcard app/models/*.c) $(wildcard app/neural/*.c) $(wildcard app/neural/*/*.c)\n\n"
        "cortex_app: $(APP_SRCS)\n"
        "\t$(CC) $(CFLAGS) $(APP_SRCS) -L$(CORTEX_ROOT) -lcortex -lm -o cortex_app\n\n"
        ".PHONY: all clean server dev assets-build\n"
        "all: cortex_app\n"
        "server: cortex_app assets-build\n"
        "\t./cortex_app\n"
        "assets-build:\n"
        "\t$(CORTEX_ROOT)/cortex assets:build\n"
        "dev:\n"
        "\t$(CORTEX_ROOT)/cortex dev\n"
        "clean:\n"
        "\trm -f cortex_app\n";
    const char *routes_content =
        "#include \"config/routes.h\"\n"
        "#include \"../action/action_request.h\"\n"
        "#include \"../action/action_response.h\"\n\n"
        "int route_get(ActionRouter *router, const char *path, ActionHandler handler) {\n"
        "    return action_router_add_route(router, \"GET\", path, handler);\n"
        "}\n\n"
        "int route_post(ActionRouter *router, const char *path, ActionHandler handler) {\n"
        "    return action_router_add_route(router, \"POST\", path, handler);\n"
        "}\n\n"
        "int route_put(ActionRouter *router, const char *path, ActionHandler handler) {\n"
        "    return action_router_add_route(router, \"PUT\", path, handler);\n"
        "}\n\n"
        "int route_delete(ActionRouter *router, const char *path, ActionHandler handler) {\n"
        "    return action_router_add_route(router, \"DELETE\", path, handler);\n"
        "}\n\n"
        "void home_index(ActionRequest *req, ActionResponse *res);\n\n"
        "void app_register_routes(ActionRouter *router) {\n"
        "    if (!router) return;\n"
        "    route_get(router, \"/\", home_index);\n"
        "}\n\n"
        "void register_routes(ActionRouter *router) {\n"
        "    app_register_routes(router);\n"
        "}\n";
    const char *home_controller_content =
        "#include \"action_controller.h\"\n"
        "#include \"action_view.h\"\n\n"
        "void home_index(ActionRequest *req, ActionResponse *res) {\n"
        "    (void)req;\n"
        "    render_view(res, \"home/index\");\n"
        "}\n";
    const char *application_layout_content =
        "<!DOCTYPE html>\n"
        "<html lang=\"en\">\n"
        "<head>\n"
        "  <meta charset=\"UTF-8\" />\n"
        "  <title>Cortex App</title>\n"
        "  <link rel=\"stylesheet\" href=\"/assets/stylesheets/application.css\" />\n"
        "</head>\n"
        "<body>\n"
        "{{yield}}\n"
        "</body>\n"
        "</html>\n";
    const char *home_view_content =
        "<!DOCTYPE html>\n"
        "<html lang=\"en\">\n"
        "<head>\n"
        "  <meta charset=\"UTF-8\" />\n"
        "  <title>Cortex — Welcome</title>\n"
        "  <link rel=\"stylesheet\" href=\"/assets/stylesheets/application.css\" />\n"
        "  <style>\n"
        "    body {\n"
        "      margin: 0;\n"
        "      padding: 0;\n"
        "      background: #0d1117;\n"
        "      color: #c9d1d9;\n"
        "      font-family: \"Courier New\", monospace;\n"
        "      display: flex;\n"
        "      align-items: center;\n"
        "      justify-content: center;\n"
        "      height: 100vh;\n"
        "    }\n"
        "\n"
        "    .container {\n"
        "      text-align: center;\n"
        "      max-width: 700px;\n"
        "      padding: 40px;\n"
        "      border: 1px solid #30363d;\n"
        "      border-radius: 12px;\n"
        "      background: #161b22;\n"
        "      box-shadow: 0 0 20px rgba(0, 0, 0, 0.6);\n"
        "    }\n"
        "\n"
        "    h1 {\n"
        "      font-size: 42px;\n"
        "      margin-bottom: 10px;\n"
        "      color: #58a6ff;\n"
        "    }\n"
        "\n"
        "    .subtitle {\n"
        "      font-size: 16px;\n"
        "      color: #8b949e;\n"
        "      margin-bottom: 30px;\n"
        "    }\n"
        "\n"
        "    .highlight {\n"
        "      color: #3fb950;\n"
        "    }\n"
        "\n"
        "    .box {\n"
        "      background: #0d1117;\n"
        "      border: 1px solid #30363d;\n"
        "      padding: 15px;\n"
        "      border-radius: 8px;\n"
        "      font-size: 14px;\n"
        "      margin: 20px 0;\n"
        "      text-align: left;\n"
        "      white-space: pre-line;\n"
        "    }\n"
        "\n"
        "    .footer {\n"
        "      margin-top: 25px;\n"
        "      font-size: 13px;\n"
        "      color: #6e7681;\n"
        "    }\n"
        "\n"
        "    .badge {\n"
        "      display: inline-block;\n"
        "      margin: 5px;\n"
        "      padding: 6px 10px;\n"
        "      border: 1px solid #30363d;\n"
        "      border-radius: 6px;\n"
        "      font-size: 12px;\n"
        "      background: #21262d;\n"
        "    }\n"
        "  </style>\n"
        "</head>\n"
        "<body>\n"
        "  <div class=\"container\">\n"
        "    <h1>⚡ Cortex</h1>\n"
        "    <div class=\"subtitle\">\n"
        "      Rails-inspired Web Framework in <span class=\"highlight\">C</span>\n"
        "    </div>\n"
        "\n"
        "    <div>\n"
        "      <span class=\"badge\">C Standard: {{C_STANDARD}}</span>\n"
        "      <span class=\"badge\">Cortex v{{CORTEX_VERSION}}</span>\n"
        "    </div>\n"
        "\n"
        "    <div class=\"box\">\n"
        "Welcome to Cortex.\n"
        "\n"
        "You are running a Rails-like MVC framework built in pure C.\n"
        "\n"
        "✔ Convention over configuration  \n"
        "✔ Generators (scaffold, models, controllers)  \n"
        "✔ MVC architecture  \n"
        "✔ Native AI (Neural layer)  \n"
        "✔ High performance, low-level control  \n"
        "\n"
        "Start building:\n"
        "\n"
        "$ cortex generate scaffold Post title:string body:text\n"
        "$ cortex db:migrate\n"
        "$ cortex server\n"
        "    </div>\n"
        "\n"
        "    <div class=\"footer\">\n"
        "      Built with low-level power ⚙️ — inspired by Rails, engineered in C.\n"
        "      <br/>\n"
        "      <a href=\"https://github.com/sergiomaia/cortex\" target=\"_blank\" rel=\"noopener noreferrer\">GitHub Repository</a>\n"
        "    </div>\n"
        "  </div>\n"
        "</body>\n"
        "</html>\n";
    const char *project_marker_content =
        "cortex_project=1\n";
    const char *runtime_js_content =
        "export class Controller {\n"
        "  constructor(context) { this.context = context; this.element = context.element; this.identifier = context.identifier; this.targetsByName = context.targetsByName; }\n"
        "  connect() {}\n"
        "  disconnect() {}\n"
        "  target(name) { const list = this.targetsByName[name] || []; return list[0] || null; }\n"
        "  targets(name) { return this.targetsByName[name] || []; }\n"
        "  hasTarget(name) { return this.targets(name).length > 0; }\n"
        "}\n"
        "export class Application {\n"
        "  constructor() { this.registry = new Map(); this.instances = []; }\n"
        "  register(identifier, ControllerClass) { this.registry.set(identifier, ControllerClass); }\n"
        "  start() { this.connectTree(document); }\n"
        "  connectTree(root) { root.querySelectorAll('[data-controller]').forEach((element) => this.connectElement(element)); }\n"
        "  connectElement(element) { const ids = (element.getAttribute('data-controller') || '').trim().split(/\\s+/).filter(Boolean); ids.forEach((identifier) => { const ControllerClass = this.registry.get(identifier); if (!ControllerClass) { console.error(`Cortex Stimulus: controller \\\"${identifier}\\\" not found`); return; } const targetsByName = this.collectTargets(identifier, element); const instance = new ControllerClass({ element, identifier, targetsByName }); this.instances.push(instance); if (typeof instance.connect === 'function') instance.connect(); this.bindActions(identifier, instance, element); }); }\n"
        "  collectTargets(identifier, element) { const map = {}; element.querySelectorAll(`[data-${identifier}-target]`).forEach((node) => { const names = (node.getAttribute(`data-${identifier}-target`) || '').split(/\\s+/).filter(Boolean); names.forEach((name) => { map[name] = map[name] || []; map[name].push(node); }); }); return map; }\n"
        "  bindActions(identifier, instance, root) { const nodes = [root, ...root.querySelectorAll('[data-action]')]; nodes.forEach((node) => { const actions = (node.getAttribute('data-action') || '').split(/\\s+/).filter(Boolean); actions.forEach((action) => { const m = action.match(/^([^->]+)->([^#]+)#(.+)$/); if (!m) { console.warn(`Cortex Stimulus: invalid action \\\"${action}\\\"`); return; } const eventName = m[1]; const actionController = m[2]; const methodName = m[3]; if (actionController !== identifier) return; if (typeof instance[methodName] !== 'function') { console.warn(`Cortex Stimulus: missing method ${identifier}#${methodName}`); return; } node.addEventListener(eventName, (event) => instance[methodName](event)); }); }); }\n"
        "}\n";

    if (!project_name || project_name[0] == '\0') {
        return -1;
    }
    if (snprintf(package_json_content, sizeof(package_json_content),
                 "{\n"
                 "  \"name\": \"%s\",\n"
                 "  \"private\": true,\n"
                 "  \"type\": \"module\",\n"
                 "  \"scripts\": {\n"
                 "    \"build\": \"cortex assets:build\",\n"
                 "    \"dev\": \"cortex dev\"\n"
                 "  },\n"
                 "  \"dependencies\": {\n"
                 "    \"react\": \"latest\",\n"
                 "    \"react-dom\": \"latest\"\n"
                 "  },\n"
                 "  \"devDependencies\": {\n"
                 "    \"esbuild\": \"latest\"\n"
                 "  }\n"
                 "}\n",
                 project_name) < 0) {
        return -1;
    }
    if (snprintf(path, sizeof(path), "%s", project_name) < 0) {
        return -1;
    }
    if (ensure_dir(path) != 0) {
        return -1;
    }
    if (snprintf(path, sizeof(path), "%s/app", project_name) < 0) {
        return -1;
    }
    if (ensure_dir(path) != 0) {
        return -1;
    }
    if (snprintf(path, sizeof(path), "%s/config", project_name) < 0) {
        return -1;
    }
    if (ensure_dir(path) != 0) {
        return -1;
    }
    /* Rails-like secret_key_base: secure key file + gitignore (see core_secret). */
    if (snprintf(path, sizeof(path), "%s/config/secret.key", project_name) < 0) {
        return -1;
    }
    if (cortex_secret_write_new_project_keyfile(path) != 0) {
        return -1;
    }
    if (snprintf(path, sizeof(path), "%s/.gitignore", project_name) < 0) {
        return -1;
    }
    if (write_template_file(path, "/config/secret.key\n") != 0) {
        return -1;
    }
    if (snprintf(path, sizeof(path), "%s/db", project_name) < 0) {
        return -1;
    }
    if (ensure_dir(path) != 0) {
        return -1;
    }
    if (snprintf(path, sizeof(path), "%s/db/migrate", project_name) < 0) {
        return -1;
    }
    if (ensure_dir(path) != 0) {
        return -1;
    }
    if (snprintf(path, sizeof(path), "%s/app/controllers", project_name) < 0) {
        return -1;
    }
    if (ensure_dir(path) != 0) {
        return -1;
    }
    if (snprintf(path, sizeof(path), "%s/app/views", project_name) < 0) {
        return -1;
    }
    if (ensure_dir(path) != 0) {
        return -1;
    }
    if (snprintf(path, sizeof(path), "%s/app/views/layouts", project_name) < 0) {
        return -1;
    }
    if (ensure_dir(path) != 0) {
        return -1;
    }
    if (snprintf(path, sizeof(path), "%s/app/views/home", project_name) < 0) {
        return -1;
    }
    if (ensure_dir(path) != 0) {
        return -1;
    }
    if (snprintf(path, sizeof(path), "%s/app/models", project_name) < 0) {
        return -1;
    }
    if (ensure_dir(path) != 0) {
        return -1;
    }
    if (snprintf(path, sizeof(path), "%s/app/neural", project_name) < 0) {
        return -1;
    }
    if (ensure_dir(path) != 0) {
        return -1;
    }
    if (snprintf(path, sizeof(path), "%s/app/javascript", project_name) < 0) {
        return -1;
    }
    if (ensure_dir(path) != 0) {
        return -1;
    }
    if (snprintf(path, sizeof(path), "%s/app/javascript/controllers", project_name) < 0) {
        return -1;
    }
    if (ensure_dir(path) != 0) {
        return -1;
    }
    if (snprintf(path, sizeof(path), "%s/app/javascript/resources", project_name) < 0) {
        return -1;
    }
    if (ensure_dir(path) != 0) {
        return -1;
    }
    if (ensure_app_assets_at(project_name) != 0) {
        return -1;
    }
    if (snprintf(path, sizeof(path), "%s/public", project_name) < 0) {
        return -1;
    }
    if (ensure_dir(path) != 0) {
        return -1;
    }
    if (snprintf(path, sizeof(path), "%s/public/assets", project_name) < 0) {
        return -1;
    }
    if (ensure_dir(path) != 0) {
        return -1;
    }
    if (snprintf(path, sizeof(path), "%s/js", project_name) < 0) {
        return -1;
    }
    if (ensure_dir(path) != 0) {
        return -1;
    }
    if (snprintf(path, sizeof(path), "%s/js/runtime", project_name) < 0) {
        return -1;
    }
    if (ensure_dir(path) != 0) {
        return -1;
    }
    if (ensure_dir(path) != 0) {
        return -1;
    }
    if (snprintf(path, sizeof(path), "%s/main.c", project_name) < 0) {
        return -1;
    }
    if (write_template_file(path, main_content) != 0) {
        return -1;
    }
    if (snprintf(path, sizeof(path), "%s/Makefile", project_name) < 0) {
        return -1;
    }
    if (write_template_file(path, makefile_content) != 0) {
        return -1;
    }
    if (snprintf(path, sizeof(path), "%s/config/routes.c", project_name) < 0) {
        return -1;
    }
    if (write_template_file(path, routes_content) != 0) {
        return -1;
    }
    if (snprintf(path, sizeof(path), "%s/app/controllers/home_controller.c", project_name) < 0) {
        return -1;
    }
    if (write_template_file(path, home_controller_content) != 0) {
        return -1;
    }
    if (snprintf(path, sizeof(path), "%s/app/views/home/index.html", project_name) < 0) {
        return -1;
    }
    if (write_template_file(path, home_view_content) != 0) {
        return -1;
    }
    if (snprintf(path, sizeof(path), "%s/app/views/layouts/application.html", project_name) < 0) {
        return -1;
    }
    if (write_template_file(path, application_layout_content) != 0) {
        return -1;
    }
    if (snprintf(path, sizeof(path), "%s/.cortex_project", project_name) < 0) {
        return -1;
    }
    if (write_template_file(path, project_marker_content) != 0) {
        return -1;
    }
    if (snprintf(path, sizeof(path), "%s/package.json", project_name) < 0) {
        return -1;
    }
    if (write_template_file(path, package_json_content) != 0) {
        return -1;
    }
    if (snprintf(path, sizeof(path), "%s/app/javascript/application.js", project_name) < 0) {
        return -1;
    }
    if (write_template_file(path,
        "import { Application } from \"../../js/runtime/index.js\";\n"
        "import { registerControllers } from \"./controllers/index.js\";\n\n"
        "const application = new Application();\n"
        "registerControllers(application);\n"
        "application.start();\n") != 0) {
        return -1;
    }
    if (snprintf(path, sizeof(path), "%s/app/javascript/application.jsx", project_name) < 0) {
        return -1;
    }
    if (write_template_file(path,
        "import React from \"react\";\n"
        "import { createRoot } from \"react-dom/client\";\n"
        "import { componentRegistry } from \"./resources/index.jsx\";\n\n"
        "function parseProps(node) {\n"
        "  const raw = node.getAttribute(\"data-react-props\") || \"{}\";\n"
        "  try { return JSON.parse(raw); } catch (_err) { return {}; }\n"
        "}\n\n"
        "function mountReactIslands() {\n"
        "  const nodes = document.querySelectorAll(\"[data-react-component]\");\n"
        "  nodes.forEach((node) => {\n"
        "    const name = node.getAttribute(\"data-react-component\");\n"
        "    const Component = componentRegistry[name];\n"
        "    if (!Component) return;\n"
        "    const root = createRoot(node);\n"
        "    root.render(React.createElement(Component, parseProps(node)));\n"
        "  });\n"
        "}\n\n"
        "if (document.readyState === \"loading\") {\n"
        "  document.addEventListener(\"DOMContentLoaded\", mountReactIslands);\n"
        "} else {\n"
        "  mountReactIslands();\n"
        "}\n") != 0) {
        return -1;
    }
    if (snprintf(path, sizeof(path), "%s/app/javascript/controllers/index.js", project_name) < 0) {
        return -1;
    }
    if (write_template_file(path,
        "/* This file is auto-generated by Cortex CLI (assets:build/dev). */\n"
        "export function registerControllers(_application) {\n"
        "  /* Controllers are registered automatically during JS build. */\n"
        "}\n") != 0) {
        return -1;
    }
    if (snprintf(path, sizeof(path), "%s/app/javascript/resources/index.jsx", project_name) < 0) {
        return -1;
    }
    if (write_template_file(path,
        "/* Auto-generated by Cortex scaffold. */\n"
        "export const componentRegistry = {};\n") != 0) {
        return -1;
    }
    if (snprintf(path, sizeof(path), "%s/js/runtime/index.js", project_name) < 0) {
        return -1;
    }
    if (write_template_file(path, runtime_js_content) != 0) {
        return -1;
    }
    if (db_create_default_in_project(project_name) != 0) {
        return -1;
    }
    return 0;
}
