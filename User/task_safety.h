#ifndef USER_TASK_SAFETY_H
#define USER_TASK_SAFETY_H

#include <stdbool.h>

/*
 * 正常跑车、开环电机测试和 PID 测试共用的纯逻辑安全判断。
 * pressedEvent 用于响应刚按下的动作；keyHeld 用于覆盖“上电前已按住”
 * 这种不会再次产生按下事件的情况。纯函数便于在电脑端直接单元测试。
 */
static inline bool TaskSafety_ShouldStop(bool motorContext,
                                         bool pressedEvent,
                                         bool keyHeld)
{
    return motorContext && (pressedEvent || keyHeld);
}

#endif /* USER_TASK_SAFETY_H */
