#ifndef ACTION_VIEW_H
#define ACTION_VIEW_H

#include "action_response.h"

typedef struct {
    const char *template_name;
} ActionView;

void action_view_render(ActionView *view, const char *data, char *buffer, int buffer_size);

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

