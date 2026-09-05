#include "semaphore.h"

void semaphore_init(semaphore_t *semaphore, int initial_value)
{
    semaphore->count = initial_value;
}

void semaphore_wait(semaphore_t *semaphore)
{
    /*
     * Wait until a resource becomes available.
     *
     * This basic implementation uses busy waiting.
     * Later, the scheduler can block the thread instead.
     */
    while (semaphore->count <= 0) {
        /* Busy wait for now. */
    }

    semaphore->count--;
}

void semaphore_signal(semaphore_t *semaphore)
{
    semaphore->count++;
}
