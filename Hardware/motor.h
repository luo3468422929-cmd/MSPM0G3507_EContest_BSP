/**
 * @file motor.h
 * @brief 定义双路 TB6612 直流电机的使能、方向、PWM、缓升和停止接口。
 *
 * 所属层：Hardware 器件层。物理引脚/PWM 由 Bsp 提供，方向、最大占空比、
 * 启动死区和缓升步长在 User/user_config.h 配置。
 */
#ifndef MOTOR_H
#define MOTOR_H

#include "common.h"

/** 左右电机逻辑编号；物理安装方向用 MOTOR_*_REVERSED 修正。 */
typedef enum {
    MOTOR_LEFT = 0, /**< 左轮 TB6612 A 通道。 */
    MOTOR_RIGHT,    /**< 右轮 TB6612 B 通道。 */
    MOTOR_COUNT     /**< 电机数量，仅用于数组和边界检查。 */
} Motor_Id_t;

/** 停止时 TB6612 方向脚的处理方式。 */
typedef enum {
    MOTOR_STOP_COAST = 0, /**< IN1=IN2=0，自由滑行。 */
    MOTOR_STOP_BRAKE      /**< IN1=IN2=1；TB6612 定义为短路制动，与 PWM 高低无关。 */
} Motor_StopMode_t;

/** @brief 清零输出、拉低 STBY 并启动 PWM 计数器。 */
Status_t Motor_Init(void);

/** @brief 控制 TB6612 STBY；关闭时立即急停而不是等待缓升。 */
Status_t Motor_Enable(bool enable);

/**
 * @brief 设置单轮逻辑目标占空比，正负号表示前进/后退。
 * @param duty 范围会限制到 ±MOTOR_MAX_DUTY，小非零值会做启动死区补偿。
 * @note 只更新目标值，实际输出由周期调用的 Motor_RampUpdate() 逐步逼近。
 */
Status_t Motor_SetDuty(Motor_Id_t id, int16_t duty);

/** @brief 同时设置左右目标占空比，任一路失败时返回对应错误。 */
Status_t Motor_SetDutyPair(int16_t leftDuty, int16_t rightDuty);

/** @brief 立即停止指定电机并选择滑行或短刹车，不经过缓升。 */
Status_t Motor_Stop(Motor_Id_t id, Motor_StopMode_t mode);

/** @brief 立即让两轮自由滑行停止，但不单独改变 STBY。 */
void Motor_StopAll(void);

/** @brief 两轮立即停转、PWM 清零并拉低 STBY，再次显式使能前不得输出。 */
void Motor_EmergencyStop(void);

/** @brief 每个控制周期调用一次，使实际占空比按 MOTOR_RAMP_STEP 接近目标。 */
void Motor_RampUpdate(void);

/** @brief 返回已施加的逻辑占空比；编号非法时返回 0。 */
int16_t Motor_GetAppliedDuty(Motor_Id_t id);

#endif

