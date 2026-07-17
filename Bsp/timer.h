/**
 * @file timer.h
 * @brief 提供基于 SysTick 的 1 ms 系统时基、经过时间和阻塞延时。
 *
 * 所属层：Bsp 时基层。周期任务应使用 Timer_GetTickMs()/ElapsedMs()，
 * 只有上电初始化等非实时场景才使用 Timer_DelayMs()。
 */
#ifndef BSP_TIMER_H
#define BSP_TIMER_H

#include "common.h"

/** @brief 配置 SysTick；reload 非法或 CMSIS 配置失败时返回错误。 */
Status_t Timer_Init(void);

/** @brief 返回上电以来的毫秒计数；约 49.7 天发生 uint32 自然回绕。 */
uint32_t Timer_GetTickMs(void);

/** @brief 返回当前 tick 与 sinceMs 的无符号差值，天然兼容一次回绕。 */
uint32_t Timer_ElapsedMs(uint32_t sinceMs);

/** @brief 使用 WFI 等待指定毫秒；依赖 SysTick 中断持续运行。 */
void Timer_DelayMs(uint32_t delayMs);

/** @brief SysTick 的可测试处理函数；每次调用将系统毫秒数加一。 */
void Timer_TickIRQHandler(void);

#endif

