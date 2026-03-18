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
 
 void test_forge_generate_controller_creates_file(void) {
     const char *name = "incident";
     const char *path = "app/controllers/incident_controller.c";
 
     remove_if_exists(path);
 
     ASSERT_EQ(forge_generate_controller(name), 0);
     ASSERT_TRUE(file_exists(path));
 }
 
 void test_forge_generate_model_creates_file(void) {
     const char *name = "incident";
     const char *path = "app/models/incident_model.c";
 
     remove_if_exists(path);
 
     ASSERT_EQ(forge_generate_model(name), 0);
     ASSERT_TRUE(file_exists(path));
 }
 
 void test_forge_generate_neural_model_creates_file(void) {
     const char *name = "incident";
     const char *path = "app/neural/incident_neural_model.c";
 
     remove_if_exists(path);
 
     ASSERT_EQ(forge_generate_neural_model(name), 0);
     ASSERT_TRUE(file_exists(path));
 }
 
