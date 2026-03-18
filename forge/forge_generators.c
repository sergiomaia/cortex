 #include "forge_generators.h"
 
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
 
 int forge_generate_model(const char *name) {
     char path[256];
     char buffer[512];
 
     if (ensure_dir("app") != 0) return -1;
     if (ensure_dir("app/models") != 0) return -1;
 
     if (snprintf(path, sizeof(path), "app/models/%s_model.c", name) < 0) {
         return -1;
     }
 
     /* Minimal model template. */
     if (snprintf(buffer, sizeof(buffer),
                  "/* Auto‑generated model: %s */\n"
                  "#include \"core/active_model.h\"\n\n"
                  "/* TODO: define fields and behaviors for %s model. */\n",
                  name, name) < 0) {
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
 
