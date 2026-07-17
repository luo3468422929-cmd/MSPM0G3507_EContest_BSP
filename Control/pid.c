/**
 * @file pid.c
 * @brief 实现带死区、积分分离、积分限幅和条件抗饱和的两种 PID。
 *
 * 所属层：Control 纯算法。所有计算使用配置中的固定 sampleTimeS；调度器
 * 应跳过错过周期而不是同一时刻连续补算。
 */
#include "pid.h"

/** 配置必须拥有正采样周期、正确上下限和非负死区/积分分离阈值。 */
static bool PID_ConfigIsValid(const PID_Config_t *config)
{
    return (config != NULL) &&
           (config->sampleTimeS > 0.0f) &&
           (config->outputMin < config->outputMax) &&
           (config->integralMin <= config->integralMax) &&
           (config->integralSeparation >= 0.0f) &&
           (config->deadband >= 0.0f);
}

static float PID_GetError(const PID_t *pid, float target, float feedback)
{
    float error = target - feedback;
    /* 死区内视作零误差，减少编码器量化噪声导致的频繁小幅调节。 */
    if ((error <= pid->config.deadband) && (error >= -pid->config.deadband)) {
        error = 0.0f;
    }
    return error;
}

Status_t PID_Init(PID_t *pid, const PID_Config_t *config)
{
    if ((pid == NULL) || !PID_ConfigIsValid(config)) {
        return STATUS_INVALID_PARAM;
    }
    pid->config = *config;
    pid->initialized = true;
    PID_Reset(pid);
    return STATUS_OK;
}

Status_t PID_SetParameters(PID_t *pid, float kp, float ki, float kd)
{
    if ((pid == NULL) || !pid->initialized) {
        return STATUS_NOT_INITIALIZED;
    }
    pid->config.kp = kp;
    pid->config.ki = ki;
    pid->config.kd = kd;
    return STATUS_OK;
}

Status_t PID_SetOutputLimits(PID_t *pid, float minimum, float maximum)
{
    if ((pid == NULL) || !pid->initialized) {
        return STATUS_NOT_INITIALIZED;
    }
    if (minimum >= maximum) {
        return STATUS_INVALID_PARAM;
    }
    pid->config.outputMin = minimum;
    pid->config.outputMax = maximum;
    pid->output = Common_ClampFloat(pid->output, minimum, maximum);
    return STATUS_OK;
}

float PID_UpdatePosition(PID_t *pid, float target, float feedback)
{
    float error;
    float derivative;
    float rawOutput;
    bool integrate;

    if ((pid == NULL) || !pid->initialized) {
        return 0.0f;
    }

    error = PID_GetError(pid, target, feedback);
    /* 大误差阶段暂停积分，避免起步/突变时积累过大。 */
    integrate = (pid->config.integralSeparation <= 0.0f) ||
                ((error < pid->config.integralSeparation) &&
                 (error > -pid->config.integralSeparation));
    if (integrate) {
        pid->integral += error * pid->config.sampleTimeS;
        pid->integral = Common_ClampFloat(pid->integral,
                                          pid->config.integralMin,
                                          pid->config.integralMax);
    }

    /* 位置式：u = Kp*e + Ki*∫e dt + Kd*de/dt。 */
    derivative = (error - pid->previousError) / pid->config.sampleTimeS;
    rawOutput = pid->config.kp * error +
                pid->config.ki * pid->integral +
                pid->config.kd * derivative;

    /* 条件积分抗饱和：输出饱和且误差继续推动饱和时撤销本次积分。 */
    if (integrate && (((rawOutput > pid->config.outputMax) && (error > 0.0f)) ||
                      ((rawOutput < pid->config.outputMin) && (error < 0.0f)))) {
        pid->integral -= error * pid->config.sampleTimeS;
        pid->integral = Common_ClampFloat(pid->integral,
                                          pid->config.integralMin,
                                          pid->config.integralMax);
        rawOutput = pid->config.kp * error +
                    pid->config.ki * pid->integral +
                    pid->config.kd * derivative;
    }

    pid->previousPreviousError = pid->previousError;
    pid->previousError = error;
    pid->output = Common_ClampFloat(rawOutput,
                                    pid->config.outputMin,
                                    pid->config.outputMax);
    return pid->output;
}

float PID_UpdateIncremental(PID_t *pid, float target, float feedback)
{
    float error;
    float delta;

    if ((pid == NULL) || !pid->initialized) {
        return 0.0f;
    }
    error = PID_GetError(pid, target, feedback);
    /* 增量式计算 Δu，再叠加到上次输出；适合只关心输出变化量的执行器。 */
    delta = pid->config.kp * (error - pid->previousError) +
            pid->config.ki * error * pid->config.sampleTimeS +
            pid->config.kd * (error - 2.0f * pid->previousError +
                              pid->previousPreviousError) / pid->config.sampleTimeS;
    pid->output = Common_ClampFloat(pid->output + delta,
                                    pid->config.outputMin,
                                    pid->config.outputMax);
    pid->previousPreviousError = pid->previousError;
    pid->previousError = error;
    return pid->output;
}

void PID_Reset(PID_t *pid)
{
    if (pid == NULL) {
        return;
    }
    pid->integral = 0.0f;
    pid->previousError = 0.0f;
    pid->previousPreviousError = 0.0f;
    pid->output = 0.0f;
}

