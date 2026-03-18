#ifndef CORE_CONTAINER_H
#define CORE_CONTAINER_H

typedef struct {
    void *data;
} CoreContainer;

CoreContainer core_container_init(void *data);
void *core_container_get(CoreContainer *container);

#endif /* CORE_CONTAINER_H */

