 #include "../test_assert.h"
 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <sys/stat.h>
 #include <errno.h>
 
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

/* Scaffold generator: Post title:string body:text */
void test_forge_scaffold_creates_model_controller_routes_fields_and_route(void) {
    const char *attrs[] = {"title:string", "body:text"};
    const int attr_count = 2;

    const char *model_path = "app/models/post.c";
    const char *controller_path = "app/controllers/posts_controller.c";
    const char *routes_path = "config/routes.c";
    const char *view_index_path = "app/views/posts/index.html";

    remove_if_exists(model_path);
    remove_if_exists(controller_path);
    remove_if_exists(routes_path);
    remove_if_exists(view_index_path);

    ASSERT_EQ(forge_generate_scaffold("Post", attr_count, attrs), 0);

    ASSERT_TRUE(file_exists(model_path));
    ASSERT_TRUE(file_exists(controller_path));
    ASSERT_TRUE(file_exists(routes_path));
    ASSERT_TRUE(file_exists(view_index_path));

    /* Fields should be parsed into active_model_set_field calls. */
    ASSERT_TRUE(file_contains(model_path, "post_set_title"));
    ASSERT_TRUE(file_contains(model_path, "active_model_set_field(m, \"title\", title)"));
    ASSERT_TRUE(file_contains(model_path, "post_set_body"));
    ASSERT_TRUE(file_contains(model_path, "active_model_set_field(m, \"body\", body)"));

    /* Controller stub should exist. */
    ASSERT_TRUE(file_contains(controller_path, "posts_controller_handle"));
    ASSERT_TRUE(file_contains(controller_path, "posts_controller_register"));
    ASSERT_TRUE(file_contains(controller_path, "action_router_add_route"));
    ASSERT_TRUE(file_contains(controller_path, "\"/posts\""));

    /* Routes should auto-register via controller register helper. */
    ASSERT_TRUE(file_contains(routes_path, "app_register_routes"));
    ASSERT_TRUE(file_contains(routes_path, "posts_controller_register"));

    remove_if_exists(model_path);
    remove_if_exists(controller_path);
    remove_if_exists(routes_path);
    remove_if_exists(view_index_path);
}
 
