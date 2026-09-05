#ifndef MUTEX_H
#define MUTEX_H

#include <stdint.h>

typedef struct {
    int locked;
    uint32_t owner;
} mutex_t;

void mutex_init(mutex_t *mutex);
void mutex_lock(mutex_t *mutex, uint32_t tid);
void mutex_unlock(mutex_t *mutex);

#endif
