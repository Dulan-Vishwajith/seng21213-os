#include "mutex.h"

void mutex_init(mutex_t *mutex)
{
    mutex->locked = 0;
    mutex->owner = 0;
}

void mutex_lock(mutex_t *mutex, uint32_t tid)
{
    /*
     * Wait until the mutex becomes available.
     *
     * This basic implementation will later be connected
     * to the scheduler so waiting threads can block.
     */
    while (mutex->locked) {
        /* Busy wait for now. */
    }

    mutex->locked = 1;
    mutex->owner = tid;
}

void mutex_unlock(mutex_t *mutex)
{
    mutex->owner = 0;
    mutex->locked = 0;
}
