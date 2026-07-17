/**
 * @file pid.h
 * @brief 定义可复用的位置式/增量式 PID、积分分离、死区和限幅接口。
 *
 * 所属层：Control 纯算法，不访问硬件。sampleTimeS 必须与实际调用周期
 * 一致；电机左右轮和循迹转向各持有独立 PID_t 状态。
 */
#ifndef PID_H
#define PID_H

#include "common.h"

/** PID 参数与所有保护范围。 */
typedef struct {
    float kp;          /**< 比例系数。 */
    float ki;          /**< 积分系数。 */
    float kd;          /**< 微分系数。 */
    float sampleTimeS; /**< 固定调用周期，单位 s。 */
    float outputMin;   /**< 输出下限。 */
    float outputMax;   /**< 输出上限。 */
    float integralMin; /**< 积分状态下限，不是积分项输出下限。 */
    float integralMax; /**< 积分状态上限。 */
    /** 当 |误差| 大于该值时暂停积分；设为 0 表示不启用积分分离。 */
    float integralSeparation;
    /** |误差| 小于死区时按零误差处理。 */
    float deadband;
} PID_Config_t;

/** 一个 PID 控制器的参数和历史状态。 */
typedef struct {
    PID_Config_t config;       /**< 当前参数副本。 */
    float integral;            /**< 位置式累计积分状态。 */
    float previousError;       /**< 上一次误差。 */
    float previousPreviousError; /**< 上上次误差，增量式微分使用。 */
    float output;              /**< 最近一次限幅后的输出。 */
    bool initialized;          /**< PID_Init 成功后为 true。 */
} PID_t;

/** @brief 校验配置、复制参数并清零状态。 */
Status_t PID_Init(PID_t *pid, const PID_Config_t *config);

/** @brief 在线修改 Kp/Ki/Kd，不清除已经积累的历史状态。 */
Status_t PID_SetParameters(PID_t *pid, float kp, float ki, float kd);

/** @brief 修改输出上下限并把当前输出立即夹紧到新范围。 */
Status_t PID_SetOutputLimits(PID_t *pid, float minimum, float maximum);

/** @brief 执行一次位置式 PID，返回限幅后的绝对输出。 */
float PID_UpdatePosition(PID_t *pid, float target, float feedback);

/** @brief 执行一次增量式 PID，在上次 output 上累加本次增量。 */
float PID_UpdateIncremental(PID_t *pid, float target, float feedback);

/** @brief 清零积分、误差历史和输出，保留配置及 initialized。 */
void PID_Reset(PID_t *pid);

#endif

