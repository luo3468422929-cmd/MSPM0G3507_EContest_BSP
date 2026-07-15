#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "common.h"

typedef enum {
    SCHEDULER_TASK_5_MS = 0,
    SCHEDULER_TASK_10_MS,
    SCHEDULER_TASK_20_MS,
    SCHEDULER_TASK_100_MS,
    SCHEDULER_TASK_200_MS,
    SCHEDULER_TASK_COUNT
} Scheduler_Task_t;

void Scheduler_Init(uint32_t nowMs);
bool Scheduler_IsDue(Scheduler_Task_t task, uint32_t nowMs);

#endif
