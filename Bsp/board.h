/**
 * @file board.h
 * @brief 提供整板统一初始化和软件急停入口。
 *
 * 所属层：Bsp。上层只需调用 Board_Init()，无需了解各模块的初始化顺序。
 * 任何初始化失败都会走 Board_EmergencyStop()，优先保证 TB6612 不输出。
 */
#ifndef BOARD_H
#define BOARD_H

#include "common.h"

/**
 * @brief 按安全顺序初始化 SysConfig、时基和所有已启用模块。
 * @return STATUS_OK 表示全部成功；否则返回第一个失败模块的状态码。
 * @note 调用失败后电机已进入急停状态，应用层不应继续启动控制任务。
 */
Status_t Board_Init(void);

/**
 * @brief 汇总板级紧急停机动作，当前会立即关闭 TB6612 输出。
 * @note 可在错误路径调用；底层本身不锁存停止状态，重新显式使能电机后可恢复。
 *       正常跑车的“只能复位恢复”策略由 User/task.c 锁存。
 */
void Board_EmergencyStop(void);

#endif

