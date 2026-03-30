 #include "../test_assert.h"
 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <sys/stat.h>
 #include <errno.h>
 #include <glob.h>
 
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
 
 void test_forge_generate_controller_creates_file(void) {
     const char *name = "incident";
     const char *path = "app/controllers/incident_controller.c";
 
     remove_if_exists(path);
 
     ASSERT_EQ(forge_generate_controller(name), 0);
     ASSERT_TRUE(file_exists(path));
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
 }

/* Scaffold generator: Post title:string email:string body:text published:boolean */
void test_forge_scaffold_creates_model_controller_routes_fields_and_route(void) {
    const char *attrs[] = {"title:string", "email:string", "body:text", "published:boolean"};
    const int attr_count = 4;

    const char *model_path = "app/models/post.c";
    const char *controller_path = "app/controllers/posts_controller.c";
    const char *routes_path = "config/routes.c";
    const char *view_index_path = "app/views/posts/index.html";
    const char *view_show_path = "app/views/posts/show.html";
    const char *view_new_path = "app/views/posts/new.html";
    const char *view_edit_path = "app/views/posts/edit.html";
    const char *layout_path = "app/views/layouts/application.html";
    const char *stimulus_controller_path = "app/javascript/controllers/post_controller.js";

    remove_if_exists(model_path);
    remove_if_exists(controller_path);
    remove_if_exists(routes_path);
    remove_if_exists(view_index_path);
    remove_if_exists(view_show_path);
    remove_if_exists(view_new_path);
    remove_if_exists(view_edit_path);
    remove_if_exists(layout_path);
    remove_glob_matches("db/migrate/*.sql");
    remove_if_exists(stimulus_controller_path);

    ASSERT_EQ(forge_generate_scaffold("Post", attr_count, attrs), 0);

    ASSERT_TRUE(file_exists(model_path));
    ASSERT_TRUE(file_exists(controller_path));
    ASSERT_TRUE(file_exists(routes_path));
    ASSERT_TRUE(file_exists(view_index_path));
    ASSERT_TRUE(file_exists(view_show_path));
    ASSERT_TRUE(file_exists(view_new_path));
    ASSERT_TRUE(file_exists(view_edit_path));
    ASSERT_TRUE(file_exists(layout_path));
    ASSERT_TRUE(file_exists(stimulus_controller_path));

    ASSERT_TRUE(file_contains(layout_path, "{{yield}}"));
    ASSERT_TRUE(any_migration_file_contains("CREATE TABLE posts"));
    ASSERT_TRUE(any_migration_file_contains("created_at"));
    ASSERT_TRUE(file_contains(view_index_path, "SQLite"));
    ASSERT_TRUE(file_contains(view_show_path, "SQLite"));
    ASSERT_TRUE(file_contains(controller_path, "FROM posts"));
    ASSERT_TRUE(file_contains(controller_path, "INSERT INTO posts"));
    ASSERT_TRUE(file_contains(controller_path, "UPDATE posts SET"));
    ASSERT_TRUE(file_contains(controller_path, "render_html"));
    ASSERT_TRUE(file_contains(controller_path, "action_view_escape_html"));

    /* Fields should be parsed into active_model_set_field calls. */
    ASSERT_TRUE(file_contains(model_path, "post_set_title"));
    ASSERT_TRUE(file_contains(model_path, "active_model_set_field(m, \"title\", title)"));
    ASSERT_TRUE(file_contains(model_path, "post_set_body"));
    ASSERT_TRUE(file_contains(model_path, "active_model_set_field(m, \"body\", body)"));

    /* Controller REST-style stubs should exist. */
    ASSERT_TRUE(file_contains(controller_path, "void posts_index("));
    ASSERT_TRUE(file_contains(controller_path, "void posts_new("));
    ASSERT_TRUE(file_contains(controller_path, "void posts_edit("));
    ASSERT_TRUE(file_contains(controller_path, "void posts_create("));
    ASSERT_TRUE(file_contains(controller_path, "void posts_update("));

    /* Routes should include resource endpoints. */
    ASSERT_TRUE(file_contains(routes_path, "app_register_routes"));
    ASSERT_TRUE(file_contains(routes_path, "route_get(router, \"/posts\", posts_index)"));
    ASSERT_TRUE(file_contains(routes_path, "route_post(router, \"/posts\", posts_create)"));
    ASSERT_TRUE(file_contains(routes_path, "route_post(router, \"/posts/:id\", posts_update)"));
    ASSERT_TRUE(file_contains(routes_path, "route_put(router, \"/posts/:id\", posts_update)"));

    /* new/edit views should include scaffold form fields from attributes. */
    ASSERT_TRUE(file_contains(view_new_path, "<form method=\"POST\" action=\"/posts\""));
    ASSERT_TRUE(file_contains(view_new_path, "data-controller=\"post\""));
    ASSERT_TRUE(file_contains(view_new_path, "data-action=\"submit->post#submit\""));
    ASSERT_TRUE(file_contains(view_new_path, "<label for=\"title\">title</label>"));
    ASSERT_TRUE(file_contains(view_new_path, "<input type=\"text\" id=\"title\" name=\"title\" />"));
    ASSERT_TRUE(file_contains(view_new_path, "<input type=\"email\" id=\"email\" name=\"email\" />"));
    ASSERT_TRUE(file_contains(view_new_path, "<label for=\"body\">body</label>"));
    ASSERT_TRUE(file_contains(view_new_path, "<textarea id=\"body\" name=\"body\"></textarea>"));
    ASSERT_TRUE(file_contains(view_new_path, "<label for=\"published\">published</label>"));
    ASSERT_TRUE(file_contains(view_new_path, "<input type=\"checkbox\" id=\"published\" name=\"published\" />"));
    ASSERT_TRUE(file_contains(view_edit_path, "<form method=\"POST\" action=\"/posts/1\""));
    ASSERT_TRUE(file_contains(view_edit_path, "data-controller=\"post\""));
    ASSERT_TRUE(file_contains(view_edit_path, "<input type=\"hidden\" name=\"_method\" value=\"PUT\" />"));
    ASSERT_TRUE(file_contains(view_edit_path, "<input type=\"text\" id=\"title\" name=\"title\" />"));
    ASSERT_TRUE(file_contains(view_edit_path, "<input type=\"email\" id=\"email\" name=\"email\" />"));
    ASSERT_TRUE(file_contains(view_edit_path, "<textarea id=\"body\" name=\"body\"></textarea>"));
    ASSERT_TRUE(file_contains(view_edit_path, "<input type=\"checkbox\" id=\"published\" name=\"published\" />"));

    remove_if_exists(model_path);
    remove_if_exists(controller_path);
    remove_if_exists(routes_path);
    remove_if_exists(view_index_path);
    remove_if_exists(view_show_path);
    remove_if_exists(view_new_path);
    remove_if_exists(view_edit_path);
    remove_if_exists(layout_path);
    remove_glob_matches("db/migrate/*.sql");
    remove_if_exists(stimulus_controller_path);
}
 
