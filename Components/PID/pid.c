#include "pid.h"

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
    integrate = (pid->config.integralSeparation <= 0.0f) ||
                ((error < pid->config.integralSeparation) &&
                 (error > -pid->config.integralSeparation));
    if (integrate) {
        pid->integral += error * pid->config.sampleTimeS;
        pid->integral = Common_ClampFloat(pid->integral,
                                          pid->config.integralMin,
                                          pid->config.integralMax);
    }

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

