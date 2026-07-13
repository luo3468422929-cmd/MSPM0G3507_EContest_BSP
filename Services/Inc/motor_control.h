#ifndef MOTOR_CONTROL_H
#define MOTOR_CONTROL_H

#include "common.h"

typedef struct {
    float targetLeftRpm;
    float targetRightRpm;
    float actualLeftRpm;
    float actualRightRpm;
    float outputLeft;
    float outputRight;
    bool enabled;
} MotorControl_Data_t;

Status_t MotorControl_Init(void);
void MotorControl_Enable(bool enable);
void MotorControl_SetTarget(float leftRpm, float rightRpm);
void MotorControl_Update(void);
const MotorControl_Data_t *MotorControl_GetData(void);
void MotorControl_Stop(void);

#endif

