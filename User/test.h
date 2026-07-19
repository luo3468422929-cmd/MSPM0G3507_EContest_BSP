/**
 * @file test.h
 * @brief 定义比赛上板时可独立选择的模块测试编号和统一运行入口。
 *
 * 所属层：User 测试框架。STARTUP_TEST 在 user_config.h 选择；测试模式
 * 优先于正常小车状态机，电机相关测试仍由 Task 提供统一急停保护。
 */
#ifndef USER_TEST_H
#define USER_TEST_H

#include "common.h"
#include "key.h"

/** 单模块测试编号；一次只能选择一个。 */
typedef enum {
    TEST_NONE = 0, /**< 不运行测试，进入正常小车状态机。 */
    TEST_LED,      /**< 板载 LED 500 ms 翻转。 */
    TEST_KEY,      /**< 按键事件通过调试 UART 输出。 */
    TEST_TRACK,    /**< 12 路循迹 mask/误差/I2C 状态输出。 */
    TEST_MOTOR,    /**< 双轮开环正停反及编码器遥测，必须架空。 */
    TEST_PID,      /**< 双轮固定目标 RPM 闭环及 VOFA 遥测。 */
    TEST_ENCODER,  /**< 手转测速和输出轴多圈 CPR 校验。 */
    TEST_IMU,      /**< yaw/gyroZ 串口与 LCD 显示。 */
    TEST_LCD,      /**< 色块、ASCII、汉字和位图一次性显示。 */
    TEST_COUNT     /**< 测试数量和非法边界。 */
} Test_Id_t;

/** @brief 切换测试并清空测试阶段状态；退出电机测试时会先急停。 */
void Test_Select(Test_Id_t id);

/** @brief 返回当前实际选中的测试；依赖未启用时选择会退回 NONE。 */
Test_Id_t Test_GetSelected(void);
/** @brief 当前测试是否会驱动电机，任务层据此启用统一急停保护。 */
bool Test_UsesMotor(Test_Id_t id);
/**
 * @brief 运行一次已选测试。
 * 按键只允许由 Task_Run 扫描一次，再把事件传入，避免多个模块抢事件。
 */
void Test_Run(uint32_t nowMs, Key_Event_t event);

#endif
