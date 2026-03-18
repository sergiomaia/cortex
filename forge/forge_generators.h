 #ifndef FORGE_GENERATORS_H
 #define FORGE_GENERATORS_H
 
 /* Simple Forge generators for app code.
  *
  * These helpers create minimal C source files under the app/ directory:
  *   - app/controllers/<name>_controller.c
  *   - app/models/<name>_model.c
  *   - app/neural/<name>_neural_model.c
  *
  * All functions return 0 on success, non‑zero on error.
  */
 
 int forge_generate_controller(const char *name);
 int forge_generate_model(const char *name);
 int forge_generate_neural_model(const char *name);
 
 #endif /* FORGE_GENERATORS_H */
 
