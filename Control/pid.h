#ifndef PID_H
#define PID_H

#include "common.h"

typedef struct {
    float kp;
    float ki;
    float kd;
    float sampleTimeS;
    float outputMin;
    float outputMax;
    float integralMin;
    float integralMax;
    /** 当 |误差| 大于该值时暂停积分；设为 0 表示不启用积分分离。 */
    float integralSeparation;
    /** |误差| 小于死区时按零误差处理。 */
    float deadband;
} PID_Config_t;

typedef struct {
    PID_Config_t config;
    float integral;
    float previousError;
    float previousPreviousError;
    float output;
    bool initialized;
} PID_t;

Status_t PID_Init(PID_t *pid, const PID_Config_t *config);
Status_t PID_SetParameters(PID_t *pid, float kp, float ki, float kd);
Status_t PID_SetOutputLimits(PID_t *pid, float minimum, float maximum);
float PID_UpdatePosition(PID_t *pid, float target, float feedback);
float PID_UpdateIncremental(PID_t *pid, float target, float feedback);
void PID_Reset(PID_t *pid);

#endif

