#include "forge_generators.h"

#include "../db/db_create.h"

#include <ctype.h>
 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <sys/stat.h>
 #include <sys/types.h>
 #include <errno.h>
 
static int ensure_javascript_structure(void);

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
    const char *app_js =
        "import { Application } from \"../../js/runtime/index.js\";\n"
        "import { registerControllers } from \"./controllers/index.js\";\n\n"
        "const application = new Application();\n"
        "registerControllers(application);\n"
        "application.start();\n";
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
    if (ensure_dir("js") != 0) return -1;
    if (ensure_dir("js/runtime") != 0) return -1;
    if (write_template_file("app/javascript/application.js", app_js) != 0) return -1;
    if (write_template_file("app/javascript/controllers/index.js", controllers_index) != 0) return -1;
    if (write_template_file("js/runtime/index.js", runtime_js) != 0) return -1;
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
    if (ensure_javascript_structure() != 0) return -1;

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
        char view_buf[4096];
        size_t view_len = 0;
        if (snprintf(path, sizeof(path), "%s/index.html", views_dir) < 0) return -1;
        if (snprintf(view_buf, sizeof(view_buf),
                     "<h1 data-controller=\"%s\" data-%s-target=\"output\">%s</h1>\n"
                     "<ul><li>Sample item</li></ul>\n"
                     "<a href=\"/%s/new\">New</a>\n",
                     resource, resource, resource_plural, resource_plural) < 0) return -1;
        if (write_template_file(path, view_buf) != 0) { perror("forge_scaffold index"); return -1; }

        if (snprintf(path, sizeof(path), "%s/show.html", views_dir) < 0) return -1;
        if (snprintf(view_buf, sizeof(view_buf),
                     "<h1>Show %s</h1>\n"
                     "<p>Sample details</p>\n"
                     "<a href=\"/%s\">Back</a>\n",
                     resource, resource_plural) < 0) return -1;
        if (write_template_file(path, view_buf) != 0) { perror("forge_scaffold show"); return -1; }

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
                 "    route_get(router, \"/%s/new\", %s_new);\n"
                 "    route_get(router, \"/%s/:id/edit\", %s_edit);\n"
                 "    route_get(router, \"/%s/:id\", %s_show);\n"
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

    if (forge_generate_stimulus_controller(resource) != 0) return -1;
    return 0;
}

int forge_new_project(const char *project_name) {
    char path[320];
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
        "        fprintf(stderr, \"database bootstrap failed\\n\");\n"
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
        "APP_SRCS := main.c config/routes.c $(wildcard app/controllers/*.c) $(wildcard app/models/*.c) $(wildcard app/neural/*.c)\n\n"
        "cortex_app: $(APP_SRCS)\n"
        "\t$(CC) $(CFLAGS) $(APP_SRCS) -L$(CORTEX_ROOT) -lcortex -lm -o cortex_app\n\n"
        ".PHONY: all clean server dev assets-build\n"
        "all: cortex_app\n"
        "server: cortex_app\n"
        "\t./cortex_app\n"
        "assets-build:\n"
        "\t./$(CORTEX_ROOT)/cortex assets:build\n"
        "dev:\n"
        "\t./$(CORTEX_ROOT)/cortex dev\n"
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
    if (snprintf(path, sizeof(path), "%s/.cortex_project", project_name) < 0) {
        return -1;
    }
    if (write_template_file(path, project_marker_content) != 0) {
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
