#include "motor_control.h"

#include "bsp_config.h"
#include "encoder.h"
#include "motor.h"
#include "pid.h"

static PID_t g_leftPid;
static PID_t g_rightPid;
static MotorControl_Data_t g_data;

Status_t MotorControl_Init(void)
{
    const PID_Config_t config = {
        .kp = 6.0f, .ki = 1.0f, .kd = 0.0f,
        .sampleTimeS = ENCODER_SAMPLE_TIME_S,
        .outputMin = -MOTOR_MAX_DUTY, .outputMax = MOTOR_MAX_DUTY,
        .integralMin = -300.0f, .integralMax = 300.0f,
        .integralSeparation = 150.0f, .deadband = 0.5f
    };
    Status_t status = PID_Init(&g_leftPid, &config);
    if (status != STATUS_OK) { return status; }
    status = PID_Init(&g_rightPid, &config);
    if (status != STATUS_OK) { return status; }
    g_data = (MotorControl_Data_t){0};
    return STATUS_OK;
}

void MotorControl_Enable(bool enable)
{
    g_data.enabled = enable;
    (void)Motor_Enable(enable);
    if (!enable) { MotorControl_Stop(); }
}

void MotorControl_SetTarget(float leftRpm, float rightRpm)
{
    g_data.targetLeftRpm = leftRpm;
    g_data.targetRightRpm = rightRpm;
}

void MotorControl_Update(void)
{
    Encoder_Data_t left;
    Encoder_Data_t right;
    Encoder_UpdateSpeed(ENCODER_SAMPLE_TIME_S);
    (void)Encoder_GetData(ENCODER_LEFT, &left);
    (void)Encoder_GetData(ENCODER_RIGHT, &right);
    g_data.actualLeftRpm = left.rpm;
    g_data.actualRightRpm = right.rpm;
    if (!g_data.enabled) { return; }
    g_data.outputLeft = PID_UpdatePosition(&g_leftPid,
        g_data.targetLeftRpm, g_data.actualLeftRpm);
    g_data.outputRight = PID_UpdatePosition(&g_rightPid,
        g_data.targetRightRpm, g_data.actualRightRpm);
    (void)Motor_SetDutyPair((int16_t)g_data.outputLeft, (int16_t)g_data.outputRight);
}

const MotorControl_Data_t *MotorControl_GetData(void) { return &g_data; }

void MotorControl_Stop(void)
{
    g_data.targetLeftRpm = 0.0f;
    g_data.targetRightRpm = 0.0f;
    g_data.outputLeft = 0.0f;
    g_data.outputRight = 0.0f;
    PID_Reset(&g_leftPid);
    PID_Reset(&g_rightPid);
    Motor_StopAll();
}

