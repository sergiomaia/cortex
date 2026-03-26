#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "action_view.h"
#include "action_response.h"
#include "action_assets.h"

/* Defined by the build system (see Makefile). Provide a safe fallback. */
#ifndef CORTEX_VERSION
#define CORTEX_VERSION "0.0.0"
#endif

static const char *c_standard_string(void) {
#if defined(__STDC_VERSION__)
    /* Map common C standard versions to friendly labels. */
    #if (__STDC_VERSION__ == 199901L)
    return "C99";
    #elif (__STDC_VERSION__ == 201112L)
    return "C11";
    #elif (__STDC_VERSION__ == 201710L)
    return "C17";
    #elif (__STDC_VERSION__ == 202311L)
    return "C23";
    #else
    return "C (unknown standard)";
    #endif
#else
    return "C";
#endif
}

static char *render_home_index_html(void) {
    const char *c_std = c_standard_string();
    const char *cortex_version = CORTEX_VERSION;

    /* This is the welcome page template with 2 dynamic values inserted. */
    const char *home_html_format =
        "<!DOCTYPE html>\n"
        "<html lang=\"en\">\n"
        "<head>\n"
        "  <meta charset=\"UTF-8\" />\n"
        "  <title>Cortex — Welcome</title>\n"
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
        "      <span class=\"badge\">C Standard: %s</span>\n"
        "      <span class=\"badge\">Cortex v%s</span>\n"
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

    int needed = snprintf(NULL, 0, home_html_format, c_std, cortex_version);
    if (needed <= 0) {
        return NULL;
    }

    char *out = (char *)malloc((size_t)needed + 1);
    if (!out) {
        return NULL;
    }
    snprintf(out, (size_t)needed + 1, home_html_format, c_std, cortex_version);
    return out;
}

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

    /* The welcome page needs dynamic values (C standard & framework version). */
    if (strcmp(template_name, "home/index") == 0) {
        char *html = render_home_index_html();
        char *html_with_assets;
        if (!html) {
            action_response_set(res, 500, "Template render error");
            action_response_set_content_type(res, "text/plain");
            return;
        }
        html_with_assets = action_assets_inject_javascript(html);
        action_response_set(res, 200, html_with_assets ? html_with_assets : html);
        action_response_set_content_type(res, "text/html");
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

    /* For all other templates, serve with JavaScript bootstrap injected. */
    {
        char *html_with_assets = action_assets_inject_javascript(buf);
        action_response_set(res, 200, html_with_assets ? html_with_assets : buf);
    }
    action_response_set_content_type(res, "text/html");
}

