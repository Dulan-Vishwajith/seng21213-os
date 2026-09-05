#ifndef THREAD_H
#define THREAD_H

#include <stdint.h>
#include "process.h"

#define MAX_THREADS 16

typedef struct {
    uint32_t tid;
    uint32_t pid;
    uint32_t esp;

    uint8_t stack[STACK_SIZE];
    process_state_t state;

    char name[24];

    void (*entry)(void);
} tcb_t;

extern tcb_t thread_table[MAX_THREADS];

void thread_init(void);

tcb_t *thread_create(
    uint32_t pid,
    const char *name,
    void (*fn)(void)
);

void thread_exit(void);

#endif
