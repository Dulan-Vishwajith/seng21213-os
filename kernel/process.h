#ifndef PROCESS_H
#define PROCESS_H

#include "../include/types.h"

#define MAX_PROCESSES 16
#define STACK_SIZE    4096

typedef enum {
    PROCESS_UNUSED = 0,
    PROCESS_READY,
    PROCESS_RUNNING,
    PROCESS_TERMINATED
} process_state_t;

typedef struct {
    uint32_t pid;
    process_state_t state;

    uint32_t esp;
    uint32_t ebp;

    uint32_t ticks;
   
    void (*entry)(void);

    uint8_t stack[STACK_SIZE];
} pcb_t;

extern pcb_t process_table[MAX_PROCESSES];

void process_init(void);
int process_create(void (*entry)(void));
void process_terminate(int pid);

#endif
