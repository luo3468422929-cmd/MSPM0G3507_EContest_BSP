#include "car_control.h"

#include "encoder.h"
#include "motor.h"
#include "pid.h"
#include "track.h"
#include "user_config.h"

static PID_t g_leftSpeedPid;
static PID_t g_rightSpeedPid;
static PID_t g_steeringPid;
static CarControl_Data_t g_data;
static uint32_t g_lastEncoderUpdateMs;
static bool g_encoderTimeInitialized;

static Status_t CarControl_InitPids(void)
{
    /* 左右轮各持有一份独立参数，电机差异明显时可分别整定。 */
    const PID_Config_t leftConfig = {
        .kp = 6.0f, .ki = 1.0f, .kd = 0.0f,
        .sampleTimeS = CONTROL_SAMPLE_TIME_S,
        .outputMin = -MOTOR_MAX_DUTY, .outputMax = MOTOR_MAX_DUTY,
        .integralMin = -300.0f, .integralMax = 300.0f,
        .integralSeparation = 150.0f, .deadband = 0.5f
    };
    const PID_Config_t rightConfig = {
        /* 右轮本轮调参起点：先关闭积分、微分，降低比例增益观察振荡。 */
        .kp = 6.0f, .ki = 1.0f, .kd = 0.0f,
        .sampleTimeS = CONTROL_SAMPLE_TIME_S,
        .outputMin = -MOTOR_MAX_DUTY, .outputMax = MOTOR_MAX_DUTY,
        .integralMin = -300.0f, .integralMax = 300.0f,
        .integralSeparation = 150.0f, .deadband = 0.5f
    };
    const PID_Config_t steeringConfig = {
        .kp = 20.0f, .ki = 0.0f, .kd = 2.0f,
        .sampleTimeS = CONTROL_SAMPLE_TIME_S,
        .outputMin = -120.0f, .outputMax = 120.0f,
        .integralMin = -20.0f, .integralMax = 20.0f,
        .integralSeparation = 3.0f, .deadband = 0.02f
    };
    Status_t status = PID_Init(&g_leftSpeedPid, &leftConfig);
    if (status != STATUS_OK) { return status; }
    status = PID_Init(&g_rightSpeedPid, &rightConfig);
    if (status != STATUS_OK) { return status; }
    return PID_Init(&g_steeringPid, &steeringConfig);
}

Status_t CarControl_Init(void)
{
    Status_t status = CarControl_InitPids();
    if (status != STATUS_OK) { return status; }
    g_data = (CarControl_Data_t){
        .baseSpeedRpm = CAR_DEFAULT_BASE_SPEED_RPM
    };
    g_lastEncoderUpdateMs = 0U;
    g_encoderTimeInitialized = false;
    return STATUS_OK;
}

void CarControl_Enable(bool enable)
{
    if (enable) {
        /* 不把停车期间的时间计入下一次速度统计窗口。 */
        g_encoderTimeInitialized = false;
        if (Motor_Enable(true) == STATUS_OK) {
            g_data.enabled = true;
        }
    } else {
        CarControl_Stop();
        (void)Motor_Enable(false);
    }
}

void CarControl_SetBaseSpeed(float rpm)
{
    g_data.baseSpeedRpm = (rpm < 0.0f) ? 0.0f : rpm;
}

static void CarControl_UpdateEncoder(uint32_t nowMs)
{
    uint32_t elapsedMs;

    if (!g_encoderTimeInitialized) {
        g_lastEncoderUpdateMs = nowMs;
        g_encoderTimeInitialized = true;
        return;
    }
    elapsedMs = (uint32_t)(nowMs - g_lastEncoderUpdateMs);
    if (elapsedMs != 0U) {
        Encoder_UpdateSpeed((float)elapsedMs / 1000.0f);
        g_lastEncoderUpdateMs = nowMs;
    }
}

static void CarControl_ReadEncoderData(Encoder_Data_t *left,
                                       Encoder_Data_t *right)
{
    (void)Encoder_GetData(ENCODER_LEFT, left);
    (void)Encoder_GetData(ENCODER_RIGHT, right);
    g_data.actualLeftRpm = left->rpm;
    g_data.actualRightRpm = right->rpm;
}

static void CarControl_RunSpeedPid(float targetLeftRpm, float targetRightRpm)
{
    g_data.targetLeftRpm = targetLeftRpm;
    g_data.targetRightRpm = targetRightRpm;
    g_data.outputLeft = PID_UpdatePosition(&g_leftSpeedPid,
        g_data.targetLeftRpm, g_data.actualLeftRpm);
    g_data.outputRight = PID_UpdatePosition(&g_rightSpeedPid,
        g_data.targetRightRpm, g_data.actualRightRpm);
    (void)Motor_SetDutyPair((int16_t)g_data.outputLeft,
                            (int16_t)g_data.outputRight);
    Motor_RampUpdate();
}

void CarControl_Update(uint32_t nowMs)
{
    Encoder_Data_t left = {0};
    Encoder_Data_t right = {0};
    const Track_Data_t *track;

    (void)Track_Update();
    CarControl_UpdateEncoder(nowMs);
    CarControl_ReadEncoderData(&left, &right);
    track = Track_GetData();

    g_data.positionError = track->positionError;
    g_data.lineFound = track->lineFound;
    if (!g_data.enabled) {
        return;
    }
    if (!g_data.lineFound) {
        /* 丢线或循迹通信失败时先停车，比赛策略可在这里替换为找线动作。 */
        g_data.targetLeftRpm = 0.0f;
        g_data.targetRightRpm = 0.0f;
        g_data.outputLeft = 0.0f;
        g_data.outputRight = 0.0f;
        PID_Reset(&g_leftSpeedPid);
        PID_Reset(&g_rightSpeedPid);
        PID_Reset(&g_steeringPid);
        Motor_StopAll();
        return;
    }

    g_data.steering = PID_UpdatePosition(&g_steeringPid, 0.0f,
                                         g_data.positionError);
    CarControl_RunSpeedPid(g_data.baseSpeedRpm - g_data.steering,
                           g_data.baseSpeedRpm + g_data.steering);
}

void CarControl_UpdateSpeedTest(uint32_t nowMs, float targetLeftRpm,
                                float targetRightRpm)
{
    Encoder_Data_t left = {0};
    Encoder_Data_t right = {0};

    CarControl_UpdateEncoder(nowMs);
    CarControl_ReadEncoderData(&left, &right);
    g_data.positionError = 0.0f;
    g_data.lineFound = true;
    g_data.steering = 0.0f;
    g_data.baseSpeedRpm = (targetLeftRpm + targetRightRpm) * 0.5f;
    if (!g_data.enabled) {
        return;
    }
    CarControl_RunSpeedPid(targetLeftRpm, targetRightRpm);
}

void CarControl_Stop(void)
{
    g_data.enabled = false;
    g_encoderTimeInitialized = false;
    g_data.targetLeftRpm = 0.0f;
    g_data.targetRightRpm = 0.0f;
    g_data.outputLeft = 0.0f;
    g_data.outputRight = 0.0f;
    PID_Reset(&g_leftSpeedPid);
    PID_Reset(&g_rightSpeedPid);
    PID_Reset(&g_steeringPid);
    Motor_StopAll();
}

const CarControl_Data_t *CarControl_GetData(void)
{
    return &g_data;
}
