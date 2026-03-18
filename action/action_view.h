#ifndef ACTION_VIEW_H
#define ACTION_VIEW_H

typedef struct {
    const char *template_name;
} ActionView;

void action_view_render(ActionView *view, const char *data, char *buffer, int buffer_size);

#endif /* ACTION_VIEW_H */

