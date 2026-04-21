 #include "../test_assert.h"
 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <sys/stat.h>
 #include <errno.h>
 #include <glob.h>
#include <unistd.h>
 
 #include "../../forge/forge_generators.h"
 
 /* Simple helper to check file existence via fopen. */
 static int file_exists(const char *path) {
     FILE *f = fopen(path, "r");
     if (f == NULL) {
         return 0;
     }
     fclose(f);
     return 1;
 }
 
 static void remove_if_exists(const char *path) {
     if (file_exists(path)) {
         remove(path);
     }
 }

 static void remove_glob_matches(const char *pattern) {
     glob_t g;
     size_t i;
     int rc;

     rc = glob(pattern, 0, NULL, &g);
     if (rc != 0 && rc != GLOB_NOMATCH) {
         return;
     }
     for (i = 0; i < g.gl_pathc; ++i) {
         remove(g.gl_pathv[i]);
     }
     globfree(&g);
 }

static void remove_scaffold_migrations_for_table(const char *table_name) {
    char pattern[256];

    if (!table_name || table_name[0] == '\0') {
        return;
    }
    if (snprintf(pattern, sizeof(pattern), "db/migrate/*_create_%s.sql", table_name) < 0) {
        return;
    }
    remove_glob_matches(pattern);
}
 
 /* Return 1 if path exists and contains substring, 0 otherwise. */
 static int file_contains(const char *path, const char *substring) {
     FILE *f;
     char buf[1024];
     size_t sublen;
     int found = 0;
 
     f = fopen(path, "r");
     if (f == NULL) return 0;
     sublen = strlen(substring);
     if (sublen == 0) { fclose(f); return 1; }
     while (fgets(buf, (int)sizeof(buf), f) != NULL && !found) {
         if (strstr(buf, substring) != NULL) found = 1;
     }
     fclose(f);
     return found;
 }

 static int any_migration_file_contains(const char *needle) {
     glob_t g;
     size_t i;
     int rc;

     rc = glob("db/migrate/*.sql", 0, NULL, &g);
     if (rc != 0 && rc != GLOB_NOMATCH) {
         return 0;
     }
     if (rc == GLOB_NOMATCH || g.gl_pathc == 0) {
         globfree(&g);
         return 0;
     }

    for (i = 0; i < g.gl_pathc; ++i) {
        if (file_contains(g.gl_pathv[i], needle)) {
            globfree(&g);
            return 1;
        }
    }
    globfree(&g);
    return 0;
}

static char *read_file_alloc(const char *path) {
    FILE *f;
    long sz;
    size_t nread;
    char *buf;

    f = fopen(path, "rb");
    if (!f) return NULL;
    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        return NULL;
    }
    sz = ftell(f);
    if (sz < 0) {
        fclose(f);
        return NULL;
    }
    if (fseek(f, 0, SEEK_SET) != 0) {
        fclose(f);
        return NULL;
    }
    buf = (char *)malloc((size_t)sz + 1);
    if (!buf) {
        fclose(f);
        return NULL;
    }
    nread = fread(buf, 1, (size_t)sz, f);
    fclose(f);
    if (nread != (size_t)sz) {
        free(buf);
        return NULL;
    }
    buf[sz] = '\0';
    return buf;
}

static int write_file_from_string(const char *path, const char *content) {
    FILE *f;
    size_t len;
    size_t nwritten;

    if (!path || !content) return -1;
    f = fopen(path, "wb");
    if (!f) return -1;
    len = strlen(content);
    nwritten = fwrite(content, 1, len, f);
    if (fclose(f) != 0) return -1;
    if (nwritten != len) return -1;
    return 0;
}
 
 void test_forge_generate_controller_creates_file(void) {
     const char *name = "incident";
    const char *path = "app/controllers/incidents_controller.c";
 
     remove_if_exists(path);
 
     ASSERT_EQ(forge_generate_controller(name), 0);
     ASSERT_TRUE(file_exists(path));
    ASSERT_TRUE(file_contains(path, "incidents_controller_handle"));

    remove_if_exists(path);
 }
 
 /* Controller generator: users -> app/controllers/users_controller.c with function stub. */
 void test_forge_generate_controller_users_creates_file_and_stub(void) {
     const char *name = "users";
     const char *path = "app/controllers/users_controller.c";
 
     remove_if_exists(path);
 
     ASSERT_EQ(forge_generate_controller(name), 0);
     ASSERT_TRUE(file_exists(path));
     ASSERT_TRUE(file_contains(path, "users_controller_handle"));
     ASSERT_TRUE(file_contains(path, "action_controller_render_text"));
 
     remove_if_exists(path);
 }
 
 void test_forge_generate_model_creates_file(void) {
     const char *name = "incident";
     const char *path = "app/models/incident.c";
 
     remove_if_exists(path);
 
     ASSERT_EQ(forge_generate_model(name), 0);
     ASSERT_TRUE(file_exists(path));

    remove_if_exists(path);
}

void test_forge_generate_model_plural_input_creates_singular_model(void) {
    const char *name = "Incidents";
    const char *path = "app/models/incident.c";
    const char *wrong_path = "app/models/incidents.c";

    remove_if_exists(path);
    remove_if_exists(wrong_path);

    ASSERT_EQ(forge_generate_model(name), 0);
    ASSERT_TRUE(file_exists(path));
    ASSERT_TRUE(!file_exists(wrong_path));

    remove_if_exists(path);
 }

void test_forge_generate_resource_creates_controller_and_views(void) {
   const char *name = "specpost";
    const char *controller_path = "app/controllers/specposts_controller.c";
    const char *index_view_path = "app/views/specposts/index.html";
    const char *show_view_path = "app/views/specposts/show.html";
    const char *new_view_path = "app/views/specposts/new.html";
    const char *edit_view_path = "app/views/specposts/edit.html";
    const char *model_path = "app/models/specpost.c";
    const char *routes_path = "config/routes.c";
    int had_routes_file = file_exists(routes_path);
    char *routes_backup = NULL;

    if (had_routes_file) {
        routes_backup = read_file_alloc(routes_path);
        ASSERT_TRUE(routes_backup != NULL);
    }

    remove_if_exists(controller_path);
    remove_if_exists(index_view_path);
    remove_if_exists(show_view_path);
    remove_if_exists(new_view_path);
    remove_if_exists(edit_view_path);
    remove_if_exists(model_path);
    if (!had_routes_file) {
        remove_if_exists(routes_path);
    }

    ASSERT_EQ(forge_generate_resource(name), 0);
    ASSERT_TRUE(file_exists(controller_path));
    ASSERT_TRUE(file_exists(index_view_path));
    ASSERT_TRUE(file_exists(show_view_path));
    ASSERT_TRUE(file_exists(new_view_path));
    ASSERT_TRUE(file_exists(edit_view_path));
    ASSERT_TRUE(!file_exists(model_path));

    remove_if_exists(controller_path);
    remove_if_exists(index_view_path);
    remove_if_exists(show_view_path);
    remove_if_exists(new_view_path);
    remove_if_exists(edit_view_path);
    if (had_routes_file && routes_backup) {
        ASSERT_EQ(write_file_from_string(routes_path, routes_backup), 0);
    } else {
        remove_if_exists(routes_path);
    }
    free(routes_backup);
}

void test_forge_generate_resource_plural_input_keeps_rails_naming(void) {
   const char *name = "Otters";
   const char *controller_path = "app/controllers/otters_controller.c";
   const char *index_view_path = "app/views/otters/index.html";
   const char *show_view_path = "app/views/otters/show.html";
   const char *new_view_path = "app/views/otters/new.html";
   const char *edit_view_path = "app/views/otters/edit.html";
   const char *wrong_controller_path = "app/controllers/otterss_controller.c";
   const char *wrong_view_dir_index_path = "app/views/otterss/index.html";
   const char *wrong_view_dir_show_path = "app/views/otterss/show.html";
   const char *wrong_view_dir_new_path = "app/views/otterss/new.html";
   const char *wrong_view_dir_edit_path = "app/views/otterss/edit.html";
   const char *model_path = "app/models/otter.c";
   const char *wrong_model_path = "app/models/otters.c";
   const char *routes_path = "config/routes.c";
   int had_routes_file = file_exists(routes_path);
   char *routes_backup = NULL;

   if (had_routes_file) {
       routes_backup = read_file_alloc(routes_path);
       ASSERT_TRUE(routes_backup != NULL);
   }

   remove_if_exists(controller_path);
   remove_if_exists(index_view_path);
   remove_if_exists(show_view_path);
   remove_if_exists(new_view_path);
   remove_if_exists(edit_view_path);
   remove_if_exists(wrong_controller_path);
   remove_if_exists(wrong_view_dir_index_path);
   remove_if_exists(wrong_view_dir_show_path);
   remove_if_exists(wrong_view_dir_new_path);
   remove_if_exists(wrong_view_dir_edit_path);
   remove_if_exists(model_path);
   remove_if_exists(wrong_model_path);
   if (!had_routes_file) {
       remove_if_exists(routes_path);
   }

   ASSERT_EQ(forge_generate_resource(name), 0);
   ASSERT_TRUE(file_exists(controller_path));
   ASSERT_TRUE(file_exists(index_view_path));
   ASSERT_TRUE(file_exists(show_view_path));
   ASSERT_TRUE(file_exists(new_view_path));
   ASSERT_TRUE(file_exists(edit_view_path));
   ASSERT_TRUE(!file_exists(wrong_controller_path));
   ASSERT_TRUE(!file_exists(wrong_view_dir_index_path));
   ASSERT_TRUE(!file_exists(wrong_view_dir_show_path));
   ASSERT_TRUE(!file_exists(wrong_view_dir_new_path));
   ASSERT_TRUE(!file_exists(wrong_view_dir_edit_path));
   ASSERT_TRUE(!file_exists(model_path));
   ASSERT_TRUE(!file_exists(wrong_model_path));

   remove_if_exists(controller_path);
   remove_if_exists(index_view_path);
   remove_if_exists(show_view_path);
   remove_if_exists(new_view_path);
   remove_if_exists(edit_view_path);
   remove_if_exists(wrong_controller_path);
   remove_if_exists(wrong_view_dir_index_path);
   remove_if_exists(wrong_view_dir_show_path);
   remove_if_exists(wrong_view_dir_new_path);
   remove_if_exists(wrong_view_dir_edit_path);
   if (had_routes_file && routes_backup) {
       ASSERT_EQ(write_file_from_string(routes_path, routes_backup), 0);
   } else {
       remove_if_exists(routes_path);
   }
   free(routes_backup);
}

void test_forge_generate_service_creates_service_file(void) {
    const char *name = "mailer";
    const char *service_path = "service/mailer.c";

    remove_if_exists(service_path);

    ASSERT_EQ(forge_generate_service(name), 0);
    ASSERT_TRUE(file_exists(service_path));
    ASSERT_TRUE(file_contains(service_path, "mailer_service_run"));

    remove_if_exists(service_path);
}
 
 /* Model generator: user -> app/models/user.c with struct and ActiveRecord. */
 void test_forge_generate_model_user_creates_file_with_fields_and_active_record(void) {
     const char *name = "user";
     const char *path = "app/models/user.c";
 
     remove_if_exists(path);
 
     ASSERT_EQ(forge_generate_model(name), 0);
     ASSERT_TRUE(file_exists(path));
     ASSERT_TRUE(file_contains(path, "typedef ActiveModel User"));
     ASSERT_TRUE(file_contains(path, "active_model_set_field"));
     ASSERT_TRUE(file_contains(path, "active_record_create"));
     ASSERT_TRUE(file_contains(path, "user_create"));
     ASSERT_TRUE(file_contains(path, "USER_MODEL_NAME"));
 
     remove_if_exists(path);
 }
 
 void test_forge_generate_neural_model_creates_file(void) {
     const char *name = "incident";
     const char *path = "app/neural/incident_neural_model.c";
 
     remove_if_exists(path);
 
     ASSERT_EQ(forge_generate_neural_model(name), 0);
     ASSERT_TRUE(file_exists(path));

    remove_if_exists(path);
}

void test_forge_generate_neural_model_plural_input_creates_singular_file(void) {
    const char *name = "Incidents";
    const char *path = "app/neural/incident_neural_model.c";
    const char *wrong_path = "app/neural/incidents_neural_model.c";

    remove_if_exists(path);
    remove_if_exists(wrong_path);

    ASSERT_EQ(forge_generate_neural_model(name), 0);
    ASSERT_TRUE(file_exists(path));
    ASSERT_TRUE(!file_exists(wrong_path));

    remove_if_exists(path);
 }

/* Scaffold generator: Post title:string email:string body:text published:boolean */
void test_forge_scaffold_creates_model_controller_routes_fields_and_route(void) {
    const char *attrs[] = {"title:string", "email:string", "body:text", "published:boolean"};
    const int attr_count = 4;

    const char *model_path = "app/models/specpost.c";
    const char *neural_path = "app/neural/specpost_neural_model.c";
    const char *controller_path = "app/controllers/specposts_controller.c";
    const char *routes_path = "config/routes.c";
    const char *view_index_path = "app/views/specposts/index.html";
    const char *view_show_path = "app/views/specposts/show.html";
    const char *view_new_path = "app/views/specposts/new.html";
    const char *view_edit_path = "app/views/specposts/edit.html";
    const char *layout_path = "app/views/layouts/application.html";
    const char *react_entry_path = "app/javascript/application.jsx";
    const char *react_registry_path = "app/javascript/resources/index.jsx";
    const char *react_resource_path = "app/javascript/resources/specposts/index.jsx";
    int had_routes_file = file_exists(routes_path);
    char *routes_backup = NULL;

    if (had_routes_file) {
        routes_backup = read_file_alloc(routes_path);
        ASSERT_TRUE(routes_backup != NULL);
    }

    remove_if_exists(model_path);
    remove_if_exists(neural_path);
    remove_if_exists(controller_path);
    if (!had_routes_file) {
        remove_if_exists(routes_path);
    }
    remove_if_exists(view_index_path);
    remove_if_exists(view_show_path);
    remove_if_exists(view_new_path);
    remove_if_exists(view_edit_path);
    remove_if_exists(layout_path);
    remove_scaffold_migrations_for_table("specposts");
    remove_if_exists(react_entry_path);
    remove_if_exists(react_registry_path);
    remove_if_exists(react_resource_path);

    ASSERT_EQ(forge_generate_scaffold("Specpost", attr_count, attrs, 1), 0);

    ASSERT_TRUE(file_exists(model_path));
    ASSERT_TRUE(file_exists(neural_path));
    ASSERT_TRUE(file_exists(controller_path));
    ASSERT_TRUE(file_exists(routes_path));
    ASSERT_TRUE(file_exists(view_index_path));
    ASSERT_TRUE(file_exists(view_show_path));
    ASSERT_TRUE(file_exists(view_new_path));
    ASSERT_TRUE(file_exists(view_edit_path));
    ASSERT_TRUE(file_exists(layout_path));
    ASSERT_TRUE(file_exists(react_entry_path));
    ASSERT_TRUE(file_exists(react_registry_path));
    ASSERT_TRUE(file_exists(react_resource_path));

    ASSERT_TRUE(file_contains(layout_path, "{{yield}}"));
    ASSERT_TRUE(any_migration_file_contains("CREATE TABLE specposts"));
    ASSERT_TRUE(any_migration_file_contains("created_at"));
    ASSERT_TRUE(file_contains(view_index_path, "data-react-component=\"specpostsIndexPage\""));
    ASSERT_TRUE(file_contains(view_index_path, "<a href=\"/specposts/new\">New Specpost</a>"));
    ASSERT_TRUE(file_contains(view_show_path, "data-react-component=\"specpostsShowPage\""));
    ASSERT_TRUE(file_contains(view_new_path, "data-react-component=\"specpostsFormPage\""));
    ASSERT_TRUE(file_contains(view_new_path, "<form method=\"POST\" action=\"/specposts\">"));
    ASSERT_TRUE(file_contains(view_edit_path, "data-react-component=\"specpostsFormPage\""));
    ASSERT_TRUE(file_contains(view_edit_path, "<form method=\"POST\" action=\"/specposts/1\">"));
    ASSERT_TRUE(file_contains(controller_path, "render_view(res, \"specposts/index\")"));
    ASSERT_TRUE(file_contains(controller_path, "render_view(res, \"specposts/show\")"));
    ASSERT_TRUE(file_contains(controller_path, "FROM specposts ORDER BY id ASC"));
    ASSERT_TRUE(file_contains(controller_path, "action_response_set_content_type(res, \"application/json\")"));
    ASSERT_TRUE(file_contains(controller_path, "INSERT INTO specposts"));
    ASSERT_TRUE(file_contains(controller_path, "UPDATE specposts SET"));

    /* Fields should be parsed into active_model_set_field calls. */
    ASSERT_TRUE(file_contains(model_path, "specpost_set_title"));
    ASSERT_TRUE(file_contains(model_path, "active_model_set_field(m, \"title\", title)"));
    ASSERT_TRUE(file_contains(model_path, "specpost_set_body"));
    ASSERT_TRUE(file_contains(model_path, "active_model_set_field(m, \"body\", body)"));
    ASSERT_TRUE(file_contains(neural_path, "specpost_neural_system_prompt"));
    ASSERT_TRUE(file_contains(neural_path, "specpost_neural_enrich"));
    ASSERT_TRUE(file_contains(neural_path, "Starting point for AI features"));

    /* Controller REST-style stubs should exist. */
    ASSERT_TRUE(file_contains(controller_path, "void specposts_index("));
    ASSERT_TRUE(file_contains(controller_path, "void specposts_new("));
    ASSERT_TRUE(file_contains(controller_path, "void specposts_edit("));
    ASSERT_TRUE(file_contains(controller_path, "void specposts_create("));
    ASSERT_TRUE(file_contains(controller_path, "void specposts_update("));

    /* Routes should include resource endpoints. */
    ASSERT_TRUE(file_contains(routes_path, "app_register_routes"));
    ASSERT_TRUE(file_contains(routes_path, "route_get(router, \"/specposts\", specposts_index)"));
    ASSERT_TRUE(file_contains(routes_path, "route_post(router, \"/specposts\", specposts_create)"));
    ASSERT_TRUE(file_contains(routes_path, "route_post(router, \"/specposts/:id\", specposts_update)"));
    ASSERT_TRUE(file_contains(routes_path, "route_put(router, \"/specposts/:id\", specposts_update)"));

    /* new/edit views should include scaffold form fields from attributes. */
    ASSERT_TRUE(file_contains(routes_path, "route_get(router, \"/specposts.json\", specposts_index)"));
    ASSERT_TRUE(file_contains(routes_path, "route_get(router, \"/specposts/:id.json\", specposts_show)"));
    ASSERT_TRUE(file_contains(react_resource_path, "export function specpostsIndexPage"));
    ASSERT_TRUE(file_contains(react_resource_path, "export function specpostsShowPage"));
    ASSERT_TRUE(file_contains(react_resource_path, "export function specpostsFormPage"));
    ASSERT_TRUE(file_contains(react_resource_path, "href: `/${resource}/${row.id}/edit`"));
    ASSERT_TRUE(file_contains(react_resource_path, "body: \"_method=DELETE\""));
    ASSERT_TRUE(file_contains(react_resource_path, "Delete this record?"));
    ASSERT_TRUE(file_contains(react_registry_path, "specpostsIndexPage: pages.specpostsIndexPage"));

    remove_if_exists(model_path);
    remove_if_exists(neural_path);
    remove_if_exists(controller_path);
    if (had_routes_file && routes_backup) {
        ASSERT_EQ(write_file_from_string(routes_path, routes_backup), 0);
    } else {
        remove_if_exists(routes_path);
    }
    remove_if_exists(view_index_path);
    remove_if_exists(view_show_path);
    remove_if_exists(view_new_path);
    remove_if_exists(view_edit_path);
    remove_if_exists(layout_path);
    remove_scaffold_migrations_for_table("specposts");
    remove_if_exists(react_entry_path);
    remove_if_exists(react_registry_path);
    remove_if_exists(react_resource_path);
    free(routes_backup);
}

void test_forge_scaffold_plural_input_uses_plural_controller_and_singular_model(void) {
    const char *attrs[] = {"title:string"};
    const int attr_count = 1;
    const char *model_path = "app/models/zebra.c";
    const char *neural_path = "app/neural/zebra_neural_model.c";
    const char *controller_path = "app/controllers/zebras_controller.c";
    const char *js_controller_path = "app/javascript/controllers/zebra_controller.js";
    const char *view_index_path = "app/views/zebras/index.html";
    const char *view_show_path = "app/views/zebras/show.html";
    const char *view_new_path = "app/views/zebras/new.html";
    const char *view_edit_path = "app/views/zebras/edit.html";
    const char *wrong_model_path = "app/models/zebras.c";
    const char *wrong_controller_path = "app/controllers/zebrass_controller.c";
    const char *wrong_view_path = "app/views/zebrass/index.html";
    const char *routes_path = "config/routes.c";
    int had_routes_file = file_exists(routes_path);
    char *routes_backup = NULL;

    if (had_routes_file) {
        routes_backup = read_file_alloc(routes_path);
        ASSERT_TRUE(routes_backup != NULL);
    }

    remove_if_exists(model_path);
    remove_if_exists(neural_path);
    remove_if_exists(controller_path);
    remove_if_exists(js_controller_path);
    remove_if_exists(view_index_path);
    remove_if_exists(view_show_path);
    remove_if_exists(view_new_path);
    remove_if_exists(view_edit_path);
    remove_if_exists(wrong_model_path);
    remove_if_exists(wrong_controller_path);
    remove_if_exists(wrong_view_path);
    if (!had_routes_file) {
        remove_if_exists(routes_path);
    }
    remove_scaffold_migrations_for_table("zebras");

    ASSERT_EQ(forge_generate_scaffold("Zebras", attr_count, attrs, 0), 0);
    ASSERT_TRUE(file_exists(model_path));
    ASSERT_TRUE(file_exists(controller_path));
    ASSERT_TRUE(file_exists(view_index_path));
    ASSERT_TRUE(!file_exists(wrong_model_path));
    ASSERT_TRUE(!file_exists(wrong_controller_path));
    ASSERT_TRUE(!file_exists(wrong_view_path));

    remove_if_exists(model_path);
    remove_if_exists(neural_path);
    remove_if_exists(controller_path);
    remove_if_exists(js_controller_path);
    remove_if_exists(view_index_path);
    remove_if_exists(view_show_path);
    remove_if_exists(view_new_path);
    remove_if_exists(view_edit_path);
    remove_scaffold_migrations_for_table("zebras");
    rmdir("app/views/zebras");
    if (had_routes_file && routes_backup) {
        ASSERT_EQ(write_file_from_string(routes_path, routes_backup), 0);
    } else {
        remove_if_exists(routes_path);
    }
    free(routes_backup);
}
 
