#include "action_view.h"

void action_view_render(ActionView *view, const char *data, char *buffer, int buffer_size) {
    (void)view;
    (void)data;
    if (!buffer || buffer_size <= 0) {
        return;
    }
    buffer[0] = '\0';
}

