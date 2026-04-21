 #ifndef FORGE_GENERATORS_H
 #define FORGE_GENERATORS_H
 
 /* Simple Forge generators for app code.
  *
  * These helpers create minimal C source files under the app/ directory:
  *   - app/controllers/<name>_controller.c
  *   - app/models/<name>.c (struct + ActiveRecord integration)
  *   - app/neural/<name>_neural_model.c
 *   - app/neural/prompts/<name>_prompt.c
 *   - app/neural/agents/<name>_agent.c
 *   - app/neural/rag/<name>_rag.c
 *   - app/neural/streams/<name>_stream.c
 *   - app/neural/memory/<name>_memory.c
 *   - app/neural/retrievers/<name>_retriever.c
 *   - app/neural/integrations/<name>_integration.c
 *   - app/neural/policies/<name>_policy.c
 *   - app/routes.c (scaffold route registration helper)
  *
  * All functions return 0 on success, non‑zero on error.
  */
 
 int forge_generate_controller(const char *name);
int forge_generate_resource(const char *name);
 int forge_generate_model(const char *name);
int forge_generate_service(const char *name);
 int forge_generate_neural_model(const char *name);
int forge_generate_neural_prompt(const char *name);
int forge_generate_neural_agent(const char *name);
int forge_generate_neural_rag(const char *name);
int forge_generate_neural_stream(const char *name);
int forge_generate_neural_memory(const char *name);
int forge_generate_neural_retriever(const char *name);
int forge_generate_neural_integration(const char *name);
int forge_generate_neural_policy(const char *name);
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
 
