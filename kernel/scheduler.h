#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "process.h"

void scheduler_init(void);
void scheduler_schedule(void);

int scheduler_get_current(void);

#endif
