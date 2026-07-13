#ifndef MOTOR_H
#define MOTOR_H

#include "common.h"

typedef enum {
    MOTOR_LEFT = 0,
    MOTOR_RIGHT,
    MOTOR_COUNT
} Motor_Id_t;

typedef enum {
    MOTOR_STOP_COAST = 0,
    MOTOR_STOP_BRAKE
} Motor_StopMode_t;

Status_t Motor_Init(void);
Status_t Motor_Enable(bool enable);
Status_t Motor_SetDuty(Motor_Id_t id, int16_t duty);
Status_t Motor_SetDutyPair(int16_t leftDuty, int16_t rightDuty);
Status_t Motor_Stop(Motor_Id_t id, Motor_StopMode_t mode);
void Motor_StopAll(void);
void Motor_EmergencyStop(void);
void Motor_RampUpdate(void);
int16_t Motor_GetAppliedDuty(Motor_Id_t id);

#endif

