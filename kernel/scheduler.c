#include "scheduler.h"

static int current_process = -1;

void scheduler_init(void)
{
    current_process = -1;
}

int scheduler_get_current(void)
{
    return current_process;
}

void scheduler_schedule(void)
{
    int i;
    int next;

    /*
     * Search for the next READY process.
     * This implements basic round-robin selection.
     */
    for (i = 1; i <= MAX_PROCESSES; i++) {

        next = (current_process + i) % MAX_PROCESSES;

        if (process_table[next].state == PROCESS_READY) {

            /*
             * If another process was running,
             * put it back into the READY state.
             */
            if (current_process >= 0 &&
                process_table[current_process].state == PROCESS_RUNNING) {

                process_table[current_process].state = PROCESS_READY;
            }

            current_process = next;

            process_table[current_process].state =
                PROCESS_RUNNING;

            process_table[current_process].ticks++;

            /*
             * Actual CPU context switching will be
             * connected later using switch.asm.
             */
            return;
        }
    }
}
