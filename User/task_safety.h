/**
 * @file task_safety.h
 * @brief 提供正常跑车和电机测试共用、可做 Host 测试的纯急停判断。
 *
 * 所属层：User 安全逻辑。独立为纯函数是为了验证“上电前已经按住启动/急停按键”
 * 这种没有新 PRESSED 事件的情况也绝不会启动电机。
 */
#ifndef USER_TASK_SAFETY_H
#define USER_TASK_SAFETY_H

#include <stdbool.h>

/**
 * @brief 判断当前电机相关上下文是否必须立即急停。
 * @param motorContext 当前处于 ARMING/RUNNING/开环电机/PID 测试之一。
 * @param pressedEvent 本轮是否产生消抖后的按下事件。
 * @param keyHeld 按键当前是否保持按下，覆盖上电前已按住情况。
 */
static inline bool TaskSafety_ShouldStop(bool motorContext,
                                         bool pressedEvent,
                                         bool keyHeld)
{
    return motorContext && (pressedEvent || keyHeld);
}

#endif /* USER_TASK_SAFETY_H */
