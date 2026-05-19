#ifndef ACTION_VIEW_H
#define ACTION_VIEW_H

#include "action_response.h"
#include "cx_context.h"

typedef struct {
    const char *template_name;
} ActionView;

typedef void (*ViewFn)(CxContext *cx);

typedef struct {
    const char *name;
    ViewFn      fn;
} ViewEntry;

#define ACTION_VIEW_REGISTRY_MAX 256

void action_view_register(const char *name, ViewFn fn);

/* Render a compiled .chtml view (and optional layout) into out. Returns 0 on success. */
int action_view_render(const char *name, CxContext *cx, char *out, int outsz);

#define CORTEX_VIEW(name_str, fn_name) \
    static void __attribute__((constructor, used)) \
    _reg_##fn_name(void) { \
        action_view_register(name_str, fn_name); \
    }

/* High-level helper: render "home/index" by loading
 * "app/views/home/index.html" and writing an HTML response.
 */
void render_view(ActionResponse *res, const char *template_name);

/* Escape &, <, >, " for safe HTML text nodes. Returns malloc'd string or NULL. */
char *action_view_escape_html(const char *input);

/* Render dynamically built HTML with the same layout + JS injection as render_view.
 * Takes ownership of inner_html (malloc'd); frees it on error paths. */
void render_html(ActionResponse *res, char *inner_html);

#endif /* ACTION_VIEW_H */
