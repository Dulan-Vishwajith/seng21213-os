#include "thread.h"

tcb_t thread_table[MAX_THREADS];

static uint32_t next_tid = 1;

void thread_init(void)
{
    int i;

    for (i = 0; i < MAX_THREADS; i++) {

        thread_table[i].tid = 0;
        thread_table[i].pid = 0;
        thread_table[i].esp = 0;

        thread_table[i].state = PROCESS_UNUSED;

        thread_table[i].name[0] = '\0';

        thread_table[i].entry = 0;
    }

    next_tid = 1;
}

tcb_t *thread_create(
    uint32_t pid,
    const char *name,
    void (*fn)(void)
)
{
    int i;
    tcb_t *thread;

    for (i = 0; i < MAX_THREADS; i++) {

        if (thread_table[i].state == PROCESS_UNUSED) {

            thread = &thread_table[i];

            thread->tid = next_tid++;
            thread->pid = pid;
            thread->state = PROCESS_READY;
            thread->entry = fn;

            /*
             * The initial stack pointer will be connected
             * to the scheduler/context-switch mechanism.
             */

	   thread->esp =
    		(uint32_t)&thread->stack[STACK_SIZE - 1];
            /*
             * Copy the thread name.
             */
            {
                int j = 0;

                while (name[j] != '\0' && j < 23) {

                    thread->name[j] = name[j];
                    j++;
                }

                thread->name[j] = '\0';
            }

            return thread;
        }
    }

    return 0;
}

void thread_exit(void)
{
    /*
     * Full thread cleanup will be connected to
     * the scheduler.
     */
}
