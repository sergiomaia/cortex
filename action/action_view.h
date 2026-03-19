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

#endif /* ACTION_VIEW_H */

