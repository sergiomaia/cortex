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

/* Generate <resource> scaffold with field setters + route registration. */
int forge_generate_scaffold(const char *resource_name, int attr_count, const char **attributes) {
    char resource[64];
    char resource_plural[64];
    char type_name[64];
    char macro_name[64];
    char path[256];

    char model_buf[4096];
    char routes_buf[1024];
    char controller_buf[2048];

    int i;
    size_t len;

    if (!resource_name || resource_name[0] == '\0') return -1;
    if (attr_count < 0) return -1;
    if (attr_count > 0 && !attributes) return -1;

    str_to_lower(resource_name, resource, sizeof(resource));
    if (resource[0] == '\0') return -1;

    /* Minimal pluralization: append 's'. */
    if (snprintf(resource_plural, sizeof(resource_plural), "%ss", resource) < 0) return -1;

    /* Derive type name (capitalize first letter) and macro name (uppercase). */
    model_type_name(resource, type_name, sizeof(type_name));
    model_macro_name(resource, macro_name, sizeof(macro_name));

    /* Ensure app + subdirs exist. */
    if (ensure_dir("app") != 0) return -1;
    if (ensure_dir("app/models") != 0) return -1;
    if (ensure_dir("app/controllers") != 0) return -1;

    /* 1) Model with field setter stubs. */
    if (snprintf(path, sizeof(path), "app/models/%s.c", resource) < 0) return -1;

    len = 0;
    len += (size_t)snprintf(model_buf + len, sizeof(model_buf) - len,
                             "/* Auto-generated scaffold model: %s */\n"
                             "#include \"core/active_record.h\"\n"
                             "#include \"core/active_model.h\"\n\n"
                             "#define %s_MODEL_NAME \"%s\"\n\n"
                             "typedef ActiveModel %s;\n\n",
                             resource, macro_name, resource, type_name);

    len += (size_t)snprintf(model_buf + len, sizeof(model_buf) - len,
                             "%s *%s_create(ActiveRecordStore *store) {\n"
                             "    return active_record_create(store, %s_MODEL_NAME);\n"
                             "}\n\n",
                             type_name, resource, macro_name);

    for (i = 0; i < attr_count; ++i) {
        const char *attr = attributes[i];
        const char *colon;
        char key[64];
        char val_name[64];

        if (!attr) return -1;
        colon = strchr(attr, ':');

        if (!colon) {
            /* No type given; treat whole token as key. */
            if (snprintf(key, sizeof(key), "%s", attr) < 0) return -1;
        } else {
            size_t klen = (size_t)(colon - attr);
            if (klen >= sizeof(key)) klen = sizeof(key) - 1;
            memcpy(key, attr, klen);
            key[klen] = '\0';
        }

        str_to_lower(key, val_name, sizeof(val_name));

        /* Best-effort: only valid identifier-ish field names are supported. */
        if (val_name[0] == '\0') return -1;

        len += (size_t)snprintf(model_buf + len, sizeof(model_buf) - len,
                                 "int %s_set_%s(%s *m, const char *%s) {\n"
                                 "    return active_model_set_field(m, \"%s\", %s);\n"
                                 "}\n\n",
                                 resource, val_name, type_name, val_name, val_name, val_name);

        if ((int)len >= (int)sizeof(model_buf) - 1) return -1;
    }

    if (write_template_file(path, model_buf) != 0) return -1;

    /* 2) Controller (resource plural) with route auto-registration helper. */
    if (snprintf(path, sizeof(path), "app/controllers/%s_controller.c", resource_plural) < 0) return -1;
    if (snprintf(controller_buf, sizeof(controller_buf),
                 "/* Auto-generated scaffold controller: %s */\n"
                 "#include \"action_controller.h\"\n"
                 "#include \"action_router.h\"\n\n"
                 "void %s_controller_handle(ActionRequest *req, ActionResponse *res) {\n"
                 "    (void)req;\n"
                 "    action_controller_render_text(res, 200, \"ok\");\n"
                 "}\n\n"
                 "void %s_controller_register(ActionRouter *router) {\n"
                 "    if (!router) return;\n"
                 "    action_router_add_route(router, \"GET\", \"/%s\", %s_controller_handle);\n"
                 "}\n",
                 resource_plural, resource_plural, resource_plural, resource_plural, resource_plural) < 0) {
        return -1;
    }
    if (write_template_file(path, controller_buf) != 0) return -1;

    /* 3) Routes file. */
    if (snprintf(path, sizeof(path), "app/routes.c") < 0) return -1;

    len = 0;
    len += (size_t)snprintf(routes_buf + len, sizeof(routes_buf) - len,
                             "/* Auto-generated routes */\n"
                             "#include \"action_router.h\"\n\n"
                             "void %s_controller_register(ActionRouter *router);\n\n"
                             "void app_routes_register(ActionRouter *router) {\n"
                             "    %s_controller_register(router);\n"
                             "}\n",
                             resource_plural, resource_plural);

    return write_template_file(path, routes_buf);
}

int forge_new_project(const char *project_name) {
    char path[320];
    const char *main_content =
        "/* Cortex app - minimal entrypoint */\n"
        "#include <stdio.h>\n\n"
        "int main(int argc, char **argv) {\n"
        "    (void)argc;\n"
        "    (void)argv;\n"
        "    printf(\"Cortex app\\n\");\n"
        "    return 0;\n"
        "}\n";
    const char *makefile_content =
        "CC := gcc\n"
        "CFLAGS := -Wall -Wextra -std=c11\n\n"
        "main: main.c\n"
        "\t$(CC) $(CFLAGS) -o main main.c\n\n"
        ".PHONY: all clean\n"
        "all: main\n"
        "clean:\n"
        "\trm -f main\n";

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
    return 0;
}
