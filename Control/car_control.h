/**
 * @file car_control.h
 * @brief 定义“循迹转向环 + 左右轮速度环 + TB6612”的整车控制接口。
 *
 * 所属层：Control 协调层。不直接调用 DriverLib；输入来自 Track/Encoder，
 * 输出统一交给 Motor。正式循迹与 TEST_PID 共用速度闭环但入口分开。
 */
#ifndef CAR_CONTROL_H
#define CAR_CONTROL_H

#include "common.h"

/** 整车控制的目标、反馈、输出和运行状态快照。 */
typedef struct {
    float baseSpeedRpm;     /**< 直行基础目标转速，单位 RPM。 */
    float targetLeftRpm;    /**< 转向修正后的左轮目标 RPM。 */
    float targetRightRpm;   /**< 转向修正后的右轮目标 RPM。 */
    float actualLeftRpm;    /**< 编码器反馈左轮 RPM。 */
    float actualRightRpm;   /**< 编码器反馈右轮 RPM。 */
    float outputLeft;       /**< 左速度 PID 输出，占空比单位。 */
    float outputRight;      /**< 右速度 PID 输出，占空比单位。 */
    float steering;         /**< 转向 PID 输出的左右 RPM 差速量。 */
    float positionError;    /**< 最近循迹位置误差。 */
    bool lineFound;         /**< 当前帧允许继续循迹。 */
    bool enabled;           /**< 控制和电机输出是否已使能。 */
} CarControl_Data_t;

/** @brief 按 user_config 参数初始化三个 PID 和控制状态。 */
Status_t CarControl_Init(void);

/** @brief 使能/关闭整车控制；关闭会清 PID、停车并关闭电机。 */
void CarControl_Enable(bool enable);

/** @brief 修改正式循迹基础速度，并限制在 0~CAR_MAX_TARGET_RPM。 */
void CarControl_SetBaseSpeed(float rpm);

/** @brief 执行一次编码器、循迹、转向和双轮速度闭环更新。 */
void CarControl_Update(uint32_t nowMs);
/**
 * @brief 仅运行左右轮速度环，跳过循迹输入，供 TEST_PID 使用。
 * @param nowMs 当前系统毫秒计数
 * @param targetLeftRpm 左轮目标转速
 * @param targetRightRpm 右轮目标转速
 */
void CarControl_UpdateSpeedTest(uint32_t nowMs, float targetLeftRpm,
                                float targetRightRpm);

/** @brief 清零目标/输出/PID 并立即停止两轮，但保留正式基础速度配置。 */
void CarControl_Stop(void);

/** @brief 返回内部控制快照的只读指针，供 LCD/串口显示。 */
const CarControl_Data_t *CarControl_GetData(void);

#endif
