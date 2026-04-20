 #ifndef FORGE_GENERATORS_H
 #define FORGE_GENERATORS_H
 
 /* Simple Forge generators for app code.
  *
  * These helpers create minimal C source files under the app/ directory:
  *   - app/controllers/<name>_controller.c
  *   - app/models/<name>.c (struct + ActiveRecord integration)
  *   - app/neural/<name>_neural_model.c
 *   - app/routes.c (scaffold route registration helper)
  *
  * All functions return 0 on success, non‑zero on error.
  */
 
 int forge_generate_controller(const char *name);
 int forge_generate_model(const char *name);
 int forge_generate_neural_model(const char *name);
int forge_generate_stimulus_controller(const char *name);
 
/* Generate a Rails-like scaffold for a resource.
 *
 * Example:
 *   forge_generate_scaffold("Post", 2, (const char*[]){"title:string","body:text"}, 1);
 *
 * Generates:
 *   - app/models/<resource>.c
 *   - app/controllers/<resource_plural>_controller.c
 *   - app/routes.c (registers GET /<resource_plural>)
 */
int forge_generate_scaffold(const char *resource_name, int attr_count, const char **attributes, int use_react);

 /* Create a new project directory with Rails-like structure: app/, config/, db/,
  * plus main.c and Makefile. Creates under current working directory.
  */
 int forge_new_project(const char *project_name);
 
 #endif /* FORGE_GENERATORS_H */
 
