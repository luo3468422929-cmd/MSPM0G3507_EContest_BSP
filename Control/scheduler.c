/**
 * @file scheduler.c
 * @brief 实现基于毫秒 tick 的固定周期门控，并在阻塞后跳过错过周期。
 *
 * 所属层：Control 调度工具。使用无符号时间差兼容 tick 回绕；它不是抢占式
 * 调度器，任务仍在 Task_Run() 中按顺序执行。
 */
#include "scheduler.h"

/** 各枚举槽位对应周期，以及上次真正放行的时间。 */
static const uint16_t g_periodMs[SCHEDULER_TASK_COUNT] = {5U, 10U, 20U, 100U, 200U};
static uint32_t g_lastRunMs[SCHEDULER_TASK_COUNT];

void Scheduler_Init(uint32_t nowMs)
{
    for (uint8_t task = 0U; task < (uint8_t)SCHEDULER_TASK_COUNT; ++task) {
        g_lastRunMs[task] = nowMs;
    }
}

bool Scheduler_IsDue(Scheduler_Task_t task, uint32_t nowMs)
{
    if (task >= SCHEDULER_TASK_COUNT) {
        return false;
    }
    if ((uint32_t)(nowMs - g_lastRunMs[task]) >= g_periodMs[task]) {
        /*
         * 主循环被 LCD/UART 阻塞后直接以实际时间重新起算。
         * 这样会跳过错过的周期，不会在同一 nowMs 连续补跑多个控制任务。
         */
        g_lastRunMs[task] = nowMs;
        return true;
    }
    return false;
}
