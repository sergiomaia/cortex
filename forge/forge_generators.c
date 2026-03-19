 #include "forge_generators.h"
 
 #include <ctype.h>
 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <sys/stat.h>
 #include <sys/types.h>
 #include <errno.h>
 
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
 
 int forge_generate_controller(const char *name) {
     char path[256];
     char buffer[512];
 
     if (ensure_dir("app") != 0) return -1;
     if (ensure_dir("app/controllers") != 0) return -1;
 
     if (snprintf(path, sizeof(path), "app/controllers/%s_controller.c", name) < 0) {
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
                  name, name) < 0) {
         return -1;
     }
 
     return write_template_file(path, buffer);
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
     char path[256];
     char type_name[64];
     char macro_name[64];
     char buffer[768];
 
     if (!name || name[0] == '\0') return -1;
     if (ensure_dir("app") != 0) return -1;
     if (ensure_dir("app/models") != 0) return -1;
 
     if (snprintf(path, sizeof(path), "app/models/%s.c", name) < 0) {
         return -1;
     }
 
     model_type_name(name, type_name, sizeof(type_name));
     model_macro_name(name, macro_name, sizeof(macro_name));
 
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
                  name, macro_name, name, type_name, type_name, name, macro_name) < 0) {
         return -1;
     }
 
     return write_template_file(path, buffer);
 }
 
 int forge_generate_neural_model(const char *name) {
     char path[256];
     char buffer[512];
 
     if (ensure_dir("app") != 0) return -1;
     if (ensure_dir("app/neural") != 0) return -1;
 
     if (snprintf(path, sizeof(path), "app/neural/%s_neural_model.c", name) < 0) {
         return -1;
     }
 
    /* Minimal neural model template. */
    if (snprintf(buffer, sizeof(buffer),
                  "/* Auto‑generated neural model: %s */\n"
                  "#include \"core/neural_model.h\"\n\n"
                  "/* TODO: configure neural model for %s. */\n",
                  name, name) < 0) {
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

/* Generate <resource> scaffold with model/controller/views/routes files. */
int forge_generate_scaffold(const char *resource_name, int attr_count, const char **attributes) {
    char resource[64];
    char resource_plural[64];
    char type_name[64];
    char macro_name[64];
    char path[256];
    char views_dir[256];
    char model_buf[4096];
    char controller_buf[2048];
    char routes_buf[4096];
    int i;
    size_t len = 0;

    if (!resource_name || resource_name[0] == '\0') return -1;
    if (attr_count < 0) return -1;
    if (attr_count > 0 && !attributes) return -1;

    str_to_lower(resource_name, resource, sizeof(resource));
    if (resource[0] == '\0') return -1;
    if (snprintf(resource_plural, sizeof(resource_plural), "%ss", resource) < 0) return -1;
    model_type_name(resource, type_name, sizeof(type_name));
    model_macro_name(resource, macro_name, sizeof(macro_name));

    if (ensure_dir("app") != 0) return -1;
    if (ensure_dir("app/models") != 0) return -1;
    if (ensure_dir("app/controllers") != 0) return -1;
    if (ensure_dir("app/views") != 0) return -1;
    if (snprintf(views_dir, sizeof(views_dir), "app/views/%s", resource_plural) < 0) return -1;
    if (ensure_dir(views_dir) != 0) return -1;
    if (ensure_dir("config") != 0) return -1;

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
        const char *colon = strchr(attr, ':');
        char key[64];
        char field[64];
        size_t klen;
        if (!attr) return -1;
        if (!colon) {
            if (snprintf(key, sizeof(key), "%s", attr) < 0) return -1;
        } else {
            klen = (size_t)(colon - attr);
            if (klen >= sizeof(key)) klen = sizeof(key) - 1;
            memcpy(key, attr, klen);
            key[klen] = '\0';
        }
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

    /* controller */
    if (snprintf(path, sizeof(path), "app/controllers/%s_controller.c", resource_plural) < 0) return -1;
    if (snprintf(controller_buf, sizeof(controller_buf),
                 "/* Auto-generated scaffold controller: %s */\n"
                 "#include \"action_controller.h\"\n"
                 "#include \"action_view.h\"\n\n"
                 "void %s_index(ActionRequest *req, ActionResponse *res) { (void)req; render_view(res, \"%s/index\"); }\n"
                 "void %s_show(ActionRequest *req, ActionResponse *res) { (void)req; render_view(res, \"%s/show\"); }\n"
                 "void %s_new(ActionRequest *req, ActionResponse *res) { (void)req; render_view(res, \"%s/new\"); }\n"
                 "void %s_edit(ActionRequest *req, ActionResponse *res) { (void)req; render_view(res, \"%s/edit\"); }\n"
                 "void %s_create(ActionRequest *req, ActionResponse *res) { (void)req; action_controller_render_text(res, 201, \"created\"); }\n"
                 "void %s_update(ActionRequest *req, ActionResponse *res) { (void)req; action_controller_render_text(res, 200, \"updated\"); }\n"
                 "void %s_delete(ActionRequest *req, ActionResponse *res) { (void)req; action_controller_render_text(res, 200, \"deleted\"); }\n",
                 resource_plural,
                 resource_plural, resource_plural,
                 resource_plural, resource_plural,
                 resource_plural, resource_plural,
                 resource_plural, resource_plural,
                 resource_plural, resource_plural, resource_plural) < 0) return -1;
    printf("forge_scaffold: generating controller at '%s'\n", path);
    if (write_template_file(path, controller_buf) != 0) { perror("forge_scaffold controller"); return -1; }

    /* views */
    {
        char view_buf[512];
        if (snprintf(path, sizeof(path), "%s/index.html", views_dir) < 0) return -1;
        if (snprintf(view_buf, sizeof(view_buf),
                     "<h1>%s</h1>\n"
                     "<ul><li>Sample item</li></ul>\n"
                     "<a href=\"/%s/new\">New</a>\n",
                     resource_plural, resource_plural) < 0) return -1;
        if (write_template_file(path, view_buf) != 0) { perror("forge_scaffold index"); return -1; }

        if (snprintf(path, sizeof(path), "%s/show.html", views_dir) < 0) return -1;
        if (snprintf(view_buf, sizeof(view_buf),
                     "<h1>Show %s</h1>\n"
                     "<p>Sample details</p>\n"
                     "<a href=\"/%s\">Back</a>\n",
                     resource, resource_plural) < 0) return -1;
        if (write_template_file(path, view_buf) != 0) { perror("forge_scaffold show"); return -1; }

        if (snprintf(path, sizeof(path), "%s/new.html", views_dir) < 0) return -1;
        if (snprintf(view_buf, sizeof(view_buf),
                     "<h1>New %s</h1>\n"
                     "<form method=\"POST\" action=\"/%s\">\n"
                     "  <button type=\"submit\">Create</button>\n"
                     "</form>\n",
                     resource, resource_plural) < 0) return -1;
        if (write_template_file(path, view_buf) != 0) { perror("forge_scaffold new"); return -1; }

        if (snprintf(path, sizeof(path), "%s/edit.html", views_dir) < 0) return -1;
        if (snprintf(view_buf, sizeof(view_buf),
                     "<h1>Edit %s</h1>\n"
                     "<form method=\"POST\" action=\"/%s/1\">\n"
                     "  <button type=\"submit\">Update</button>\n"
                     "</form>\n",
                     resource, resource_plural) < 0) return -1;
        if (write_template_file(path, view_buf) != 0) { perror("forge_scaffold edit"); return -1; }
    }

    /* routes */
    if (snprintf(path, sizeof(path), "config/routes.c") < 0) return -1;
    if (snprintf(routes_buf, sizeof(routes_buf),
                 "#include \"config/routes.h\"\n"
                 "#include \"../action/action_request.h\"\n"
                 "#include \"../action/action_response.h\"\n\n"
                 "void %s_index(ActionRequest *req, ActionResponse *res);\n"
                 "void %s_show(ActionRequest *req, ActionResponse *res);\n"
                 "void %s_new(ActionRequest *req, ActionResponse *res);\n"
                 "void %s_edit(ActionRequest *req, ActionResponse *res);\n"
                 "void %s_create(ActionRequest *req, ActionResponse *res);\n"
                 "void %s_update(ActionRequest *req, ActionResponse *res);\n"
                 "void %s_delete(ActionRequest *req, ActionResponse *res);\n\n"
                 "void home_index(ActionRequest *req, ActionResponse *res);\n\n"
                 "void app_register_routes(ActionRouter *router) {\n"
                 "    if (!router) return;\n"
                 "    route_get(router, \"/\", home_index);\n"
                 "    route_get(router, \"/%s\", %s_index);\n"
                 "    route_get(router, \"/%s/:id\", %s_show);\n"
                 "    route_get(router, \"/%s/new\", %s_new);\n"
                 "    route_get(router, \"/%s/:id/edit\", %s_edit);\n"
                 "    route_post(router, \"/%s\", %s_create);\n"
                 "    route_put(router, \"/%s/:id\", %s_update);\n"
                 "    route_delete(router, \"/%s/:id\", %s_delete);\n"
                 "}\n",
                 resource_plural, resource_plural, resource_plural, resource_plural, resource_plural, resource_plural, resource_plural,
                 resource_plural, resource_plural,
                 resource_plural, resource_plural,
                 resource_plural, resource_plural,
                 resource_plural, resource_plural,
                 resource_plural, resource_plural,
                 resource_plural, resource_plural,
                 resource_plural, resource_plural) < 0) return -1;
    printf("forge_scaffold: updating routes in '%s'\n", path);
    if (write_template_file(path, routes_buf) != 0) { perror("forge_scaffold routes"); return -1; }

    return 0;
}

int forge_new_project(const char *project_name) {
    char path[320];
    const char *main_content =
        "/* Cortex app server entrypoint */\n"
        "#include <stdio.h>\n"
        "#include \"action/action_dispatch.h\"\n"
        "#include \"action/action_router.h\"\n"
        "#include \"config/routes.h\"\n\n"
        "void app_register_routes(ActionRouter *router);\n\n"
        "int main(int argc, char **argv) {\n"
        "    (void)argc;\n"
        "    (void)argv;\n"
        "    ActionRouter router;\n"
        "    action_router_init(&router);\n"
        "    app_register_routes(&router);\n"
        "    printf(\"Listening on http://localhost:3000\\n\");\n"
        "    return action_dispatch_serve_http(&router);\n"
        "}\n";
    const char *makefile_content =
        "CC := gcc\n"
        "CORTEX_ROOT ?= ..\n"
        "CFLAGS := -Wall -Wextra -std=c11 -I. -I$(CORTEX_ROOT) -I$(CORTEX_ROOT)/core -I$(CORTEX_ROOT)/action -I$(CORTEX_ROOT)/config\n"
        "APP_SRCS := main.c config/routes.c $(wildcard app/controllers/*.c) $(wildcard app/models/*.c) $(wildcard app/neural/*.c)\n\n"
        "cortex_app: $(APP_SRCS)\n"
        "\t$(CC) $(CFLAGS) $(APP_SRCS) -L$(CORTEX_ROOT) -lcortex -lm -o cortex_app\n\n"
        ".PHONY: all clean server\n"
        "all: cortex_app\n"
        "server: cortex_app\n"
        "\t./cortex_app\n"
        "clean:\n"
        "\trm -f cortex_app\n";
    const char *routes_content =
        "#include \"config/routes.h\"\n"
        "#include \"../action/action_request.h\"\n"
        "#include \"../action/action_response.h\"\n\n"
        "void home_index(ActionRequest *req, ActionResponse *res);\n\n"
        "void app_register_routes(ActionRouter *router) {\n"
        "    if (!router) return;\n"
        "    route_get(router, \"/\", home_index);\n"
        "}\n";
    const char *home_controller_content =
        "#include \"action_controller.h\"\n"
        "#include \"action_view.h\"\n\n"
        "void home_index(ActionRequest *req, ActionResponse *res) {\n"
        "    (void)req;\n"
        "    render_view(res, \"home/index\");\n"
        "}\n";
    const char *home_view_content =
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

    if (!project_name || project_name[0] == '\0') {
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
    if (snprintf(path, sizeof(path), "%s/db", project_name) < 0) {
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
    if (snprintf(path, sizeof(path), "%s/.cortex_project", project_name) < 0) {
        return -1;
    }
    if (write_template_file(path, project_marker_content) != 0) {
        return -1;
    }
    return 0;
}
