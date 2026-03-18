#include "core_container.h"

CoreContainer core_container_init(void *data) {
    CoreContainer c;
    c.data = data;
    return c;
}

void *core_container_get(CoreContainer *container) {
    if (!container) {
        return 0;
    }
    return container->data;
}

