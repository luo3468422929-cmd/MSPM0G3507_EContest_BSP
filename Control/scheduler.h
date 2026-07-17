/**
 * @file scheduler.h
 * @brief 定义无 RTOS 主循环中的 5/10/20/100/200 ms 周期门控器。
 *
 * 所属层：Control 调度工具。它只判断“本轮是否到期”，不保存函数指针；
 * 具体任务仍由 User/task.c 按优先级调用。
 */
#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "common.h"

/** 固定周期槽位；枚举值用作内部数组下标。 */
typedef enum {
    SCHEDULER_TASK_5_MS = 0, /**< 按键消抖。 */
    SCHEDULER_TASK_10_MS,   /**< 编码器/循迹/控制。 */
    SCHEDULER_TASK_20_MS,   /**< 惯导和命令解析。 */
    SCHEDULER_TASK_100_MS,  /**< LCD 分行和低频遥测。 */
    SCHEDULER_TASK_200_MS,  /**< 预留更低频任务。 */
    SCHEDULER_TASK_COUNT    /**< 周期槽数量。 */
} Scheduler_Task_t;

/** @brief 把所有槽位的上次执行时刻设置为 nowMs。 */
void Scheduler_Init(uint32_t nowMs);

/** @brief 到期时更新时间戳并返回 true；非法槽位或未到期返回 false。 */
bool Scheduler_IsDue(Scheduler_Task_t task, uint32_t nowMs);

#endif
