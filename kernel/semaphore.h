#ifndef SEMAPHORE_H
#define SEMAPHORE_H

#include <stdint.h>

typedef struct {
    int count;
} semaphore_t;

void semaphore_init(semaphore_t *semaphore, int initial_value);

void semaphore_wait(semaphore_t *semaphore);

void semaphore_signal(semaphore_t *semaphore);

#endif
