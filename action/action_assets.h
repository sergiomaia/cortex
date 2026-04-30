#ifndef ACTION_ASSETS_H
#define ACTION_ASSETS_H

/* Returns "/assets/<compiled-name>" from manifest or fallback.
 * The returned pointer is stable across calls.
 */
const char *action_assets_javascript_path(void);

/* Serves /assets/... from public/assets first, then app/assets (Rails-like source tree).
 * Returns 0 if served, -1 otherwise.
 */
int action_assets_serve_static_path(const char *request_path, int client_fd);

/* Injects the script tag before </body> when HTML is detected.
 * Returns malloc'ed output that caller owns, or NULL on failure.
 */
char *action_assets_inject_javascript(const char *html);

#endif /* ACTION_ASSETS_H */
