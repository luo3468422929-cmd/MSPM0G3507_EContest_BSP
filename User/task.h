/**
 * @file task.h
 * @brief 定义板级初始化包装、任务状态初始化和裸机主循环单步入口。
 *
 * 所属层：User。main() 只调用这三个函数；具体状态机、周期分工和测试
 * 优先级均封装在 task.c。
 */
#ifndef USER_TASK_H
#define USER_TASK_H

#include "common.h"

/** @brief 调用 Board_Init() 完成整板初始化。 */
Status_t System_Init(void);

/** @brief 初始化控制、调度、测试选择和正常启动状态机。 */
Status_t Task_Init(void);

/**
 * @brief 执行一次单步协同轮询；main() 在永久循环中反复调用。
 * @note 总线操作可能在有限超时内短暂阻塞，但不应包含无限等待。
 */
void Task_Run(void);

#endif
