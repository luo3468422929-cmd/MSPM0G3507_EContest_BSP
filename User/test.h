#ifndef USER_TEST_H
#define USER_TEST_H

#include "common.h"
#include "key.h"

typedef enum {
    TEST_NONE = 0,
    TEST_LED,
    TEST_KEY,
    TEST_TRACK,
    TEST_MOTOR,
    TEST_PID,
    TEST_ENCODER,
    TEST_IMU,
    TEST_LCD,
    TEST_COUNT
} Test_Id_t;

void Test_Select(Test_Id_t id);
Test_Id_t Test_GetSelected(void);
/** 当前测试是否会驱动电机，任务层据此启用统一急停保护。 */
bool Test_UsesMotor(Test_Id_t id);
/**
 * 运行一次已选测试。
 * 按键只允许由 Task_Run 扫描一次，再把事件传入，避免多个模块抢事件。
 */
void Test_Run(uint32_t nowMs, Key_Event_t event);

#endif
