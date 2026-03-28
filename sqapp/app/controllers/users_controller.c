#define _POSIX_C_SOURCE 200809L

#include "action_controller.h"
#include "action_response.h"
#include "action_assets.h"
#include "db/db_bootstrap.h"
#include "db/db_connection.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int ensure_users_schema(void) {
    const char *sql =
        "CREATE TABLE IF NOT EXISTS users ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "name TEXT NOT NULL DEFAULT '',"
        "email TEXT NOT NULL DEFAULT '',"
        "info TEXT NOT NULL DEFAULT '',"
        "\"true\" INTEGER NOT NULL DEFAULT 0"
        ");";
    if (cortex_db_exec(sql) != 0) {
        return -1;
    }
    /* Seed one row if the table is empty (development convenience). */
    return cortex_db_exec(
        "INSERT INTO users (name, email, info, \"true\") "
        "SELECT 'Maria', 'maria@example.com', 'Perfil de demonstração', 1 "
        "WHERE NOT EXISTS (SELECT 1 FROM users LIMIT 1)");
}

static char *append_str(char *base, const char *chunk) {
    size_t bl = base ? strlen(base) : 0;
    size_t cl = strlen(chunk);
    char *out = (char *)realloc(base, bl + cl + 1);
    if (!out) {
        free(base);
        return NULL;
    }
    memcpy(out + bl, chunk, cl + 1);
    return out;
}

static void html_escape_into(const char *src, char *dst, size_t dst_cap) {
    size_t w = 0;
    if (!src || !dst || dst_cap == 0) {
        if (dst && dst_cap > 0) {
            dst[0] = '\0';
        }
        return;
    }
    for (; *src && w + 1 < dst_cap; src++) {
        const char *rep = NULL;
        if (*src == '&') {
            rep = "&amp;";
        } else if (*src == '<') {
            rep = "&lt;";
        } else if (*src == '>') {
            rep = "&gt;";
        } else if (*src == '"') {
            rep = "&quot;";
        }
        if (rep) {
            size_t rl = strlen(rep);
            if (w + rl >= dst_cap) {
                break;
            }
            memcpy(dst + w, rep, rl);
            w += rl;
        } else {
            dst[w++] = *src;
        }
    }
    dst[w] = '\0';
}

static char *read_template_file(void) {
    FILE *f = fopen("app/views/users/index.html", "rb");
    long len;
    char *buf;

    if (!f) {
        return NULL;
    }
    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        return NULL;
    }
    len = ftell(f);
    if (len < 0) {
        fclose(f);
        return NULL;
    }
    if (fseek(f, 0, SEEK_SET) != 0) {
        fclose(f);
        return NULL;
    }
    buf = (char *)malloc((size_t)len + 1);
    if (!buf) {
        fclose(f);
        return NULL;
    }
    if (len > 0 && fread(buf, 1, (size_t)len, f) != (size_t)len) {
        free(buf);
        fclose(f);
        return NULL;
    }
    buf[len] = '\0';
    fclose(f);
    return buf;
}

static char *build_user_rows(void) {
    DbConnection *db = cortex_db_connection();
    DbStatement *stmt = NULL;
    char *rows = strdup("");
    char cell_name[512];
    char cell_email[512];
    char cell_info[1024];

    if (!db || !rows) {
        free(rows);
        return strdup("<tr><td colspan=\"4\">Erro ao conectar ao banco.</td></tr>");
    }

    if (db_connection_prepare(db, "SELECT name, email, info, \"true\" FROM users ORDER BY id", &stmt) != 0) {
        free(rows);
        return strdup("<tr><td colspan=\"4\">Erro na consulta.</td></tr>");
    }

    while (db_statement_step(stmt) == 1) {
        const char *name = db_statement_column_text(stmt, 0);
        const char *email = db_statement_column_text(stmt, 1);
        const char *info = db_statement_column_text(stmt, 2);
        int t = db_statement_column_int(stmt, 3);
        char row[4096];

        html_escape_into(name ? name : "", cell_name, sizeof(cell_name));
        html_escape_into(email ? email : "", cell_email, sizeof(cell_email));
        html_escape_into(info ? info : "", cell_info, sizeof(cell_info));

        snprintf(row, sizeof(row),
                 "<tr><td>%s</td><td>%s</td><td>%s</td><td>%s</td></tr>\n",
                 cell_name, cell_email, cell_info, t ? "true" : "false");
        {
            char *next = append_str(rows, row);
            if (!next) {
                db_statement_finalize(stmt);
                return strdup("<tr><td colspan=\"4\">Erro de memória.</td></tr>");
            }
            rows = next;
        }
    }
    db_statement_finalize(stmt);

    if (rows[0] == '\0') {
        free(rows);
        return strdup("<tr><td colspan=\"4\">Nenhum registro.</td></tr>");
    }
    return rows;
}

static char *substitute_placeholder(char *template_html, const char *needle, const char *replacement) {
    char *pos = strstr(template_html, needle);
    size_t nlen = strlen(needle);
    size_t rlen = strlen(replacement);
    size_t prefix;
    char *out;

    if (!pos) {
        return template_html;
    }
    prefix = (size_t)(pos - template_html);
    out = (char *)malloc(prefix + rlen + strlen(pos + nlen) + 1);
    if (!out) {
        free(template_html);
        return NULL;
    }
    memcpy(out, template_html, prefix);
    memcpy(out + prefix, replacement, rlen);
    strcpy(out + prefix + rlen, pos + nlen);
    free(template_html);
    return out;
}

void users_index(ActionRequest *req, ActionResponse *res) {
    char *html;
    char *rows;
    char *with_rows;
    char *final_html;

    (void)req;

    if (ensure_users_schema() != 0) {
        action_response_set(res, 500, "Falha ao preparar tabela users");
        action_response_set_content_type(res, "text/plain; charset=utf-8");
        return;
    }

    html = read_template_file();
    if (!html) {
        action_response_set(res, 500, "Template users/index não encontrado");
        action_response_set_content_type(res, "text/plain; charset=utf-8");
        return;
    }

    rows = build_user_rows();
    if (!rows) {
        free(html);
        action_response_set(res, 500, "Erro ao montar linhas");
        action_response_set_content_type(res, "text/plain; charset=utf-8");
        return;
    }

    with_rows = substitute_placeholder(html, "{{USER_ROWS}}", rows);
    free(rows);
    if (!with_rows) {
        action_response_set(res, 500, "Erro ao substituir template");
        action_response_set_content_type(res, "text/plain; charset=utf-8");
        return;
    }

    final_html = action_assets_inject_javascript(with_rows);
    if (final_html) {
        free(with_rows);
        with_rows = final_html;
    }

    action_response_set(res, 200, with_rows);
    action_response_set_content_type(res, "text/html; charset=utf-8");
}
