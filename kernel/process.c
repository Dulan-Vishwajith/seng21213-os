#include "process.h"

pcb_t process_table[MAX_PROCESSES];

static uint32_t next_pid = 1;

void process_init(void)
{
    int i;

    next_pid = 1;

    for (i = 0; i < MAX_PROCESSES; i++) {
        process_table[i].pid = 0;
        process_table[i].state = PROCESS_UNUSED;
        process_table[i].esp = 0;
        process_table[i].ebp = 0;
        process_table[i].ticks = 0;
        process_table[i].entry = NULL;
    }
}

int process_create(void (*entry)(void))
{
    int i;

    for (i = 0; i < MAX_PROCESSES; i++) {

        if (process_table[i].state == PROCESS_UNUSED) {

            process_table[i].pid = next_pid++;
            process_table[i].state = PROCESS_READY;
            process_table[i].ticks = 0;

            /*
             * Stack grows downward.
             * Start the process at the top of its stack.
             */
            process_table[i].esp =
                (uint32_t)&process_table[i].stack[STACK_SIZE - 1];

            process_table[i].ebp =
                process_table[i].esp;

            process_table[i].entry = entry;

            return i;
        }
    }

    return -1;
}

void process_terminate(int pid)
{
    int i;

    for (i = 0; i < MAX_PROCESSES; i++) {

        if (process_table[i].pid == (uint32_t)pid &&
            process_table[i].state != PROCESS_UNUSED) {
            process_table[i].entry = NULL;
            process_table[i].state = PROCESS_TERMINATED;
            return;
        }
    }
}
