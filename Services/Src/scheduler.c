#include "scheduler.h"

static const uint16_t g_periodMs[SCHEDULER_TASK_COUNT] = {5U, 10U, 20U, 100U};
static uint32_t g_lastRunMs[SCHEDULER_TASK_COUNT];

void Scheduler_Init(uint32_t nowMs)
{
    for (uint8_t task = 0U; task < (uint8_t)SCHEDULER_TASK_COUNT; ++task) {
        g_lastRunMs[task] = nowMs;
    }
}

bool Scheduler_IsDue(Scheduler_Task_t task, uint32_t nowMs)
{
    if ((task < SCHEDULER_TASK_5_MS) || (task >= SCHEDULER_TASK_COUNT)) {
        return false;
    }
    if ((uint32_t)(nowMs - g_lastRunMs[task]) >= g_periodMs[task]) {
        /* 按周期推进而非直接赋 now，降低主循环抖动造成的长期漂移。 */
        g_lastRunMs[task] += g_periodMs[task];
        return true;
    }
    return false;
}

