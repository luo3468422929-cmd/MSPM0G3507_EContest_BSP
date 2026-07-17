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
        .kp = SPEED_PID_LEFT_KP,
        .ki = SPEED_PID_LEFT_KI,
        .kd = SPEED_PID_LEFT_KD,
        .sampleTimeS = CONTROL_SAMPLE_TIME_S,
        .outputMin = -MOTOR_MAX_DUTY, .outputMax = MOTOR_MAX_DUTY,
        .integralMin = -SPEED_PID_INTEGRAL_LIMIT,
        .integralMax = SPEED_PID_INTEGRAL_LIMIT,
        .integralSeparation = SPEED_PID_INTEGRAL_SEPARATION,
        .deadband = SPEED_PID_DEADBAND
    };
    const PID_Config_t rightConfig = {
        .kp = SPEED_PID_RIGHT_KP,
        .ki = SPEED_PID_RIGHT_KI,
        .kd = SPEED_PID_RIGHT_KD,
        .sampleTimeS = CONTROL_SAMPLE_TIME_S,
        .outputMin = -MOTOR_MAX_DUTY, .outputMax = MOTOR_MAX_DUTY,
        .integralMin = -SPEED_PID_INTEGRAL_LIMIT,
        .integralMax = SPEED_PID_INTEGRAL_LIMIT,
        .integralSeparation = SPEED_PID_INTEGRAL_SEPARATION,
        .deadband = SPEED_PID_DEADBAND
    };
    const PID_Config_t steeringConfig = {
        .kp = STEERING_PID_KP,
        .ki = STEERING_PID_KI,
        .kd = STEERING_PID_KD,
        .sampleTimeS = CONTROL_SAMPLE_TIME_S,
        .outputMin = -STEERING_OUTPUT_LIMIT,
        .outputMax = STEERING_OUTPUT_LIMIT,
        .integralMin = -STEERING_INTEGRAL_LIMIT,
        .integralMax = STEERING_INTEGRAL_LIMIT,
        .integralSeparation = STEERING_INTEGRAL_SEPARATION,
        .deadband = STEERING_DEADBAND
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
    g_data.baseSpeedRpm = Common_ClampFloat(rpm, 0.0f,
                                            CAR_MAX_TARGET_RPM);
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
    const float minimumRpm = (CAR_ALLOW_REVERSE_WHEEL != 0) ?
                             -CAR_MAX_TARGET_RPM : 0.0f;

    g_data.targetLeftRpm = Common_ClampFloat(
        targetLeftRpm, minimumRpm, CAR_MAX_TARGET_RPM);
    g_data.targetRightRpm = Common_ClampFloat(
        targetRightRpm, minimumRpm, CAR_MAX_TARGET_RPM);
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

    /*
     * 先用本轮入口时间更新编码器，再执行可能阻塞并重试的 I2C 循迹读取。
     * 这样 I2C 超时不会让本次编码器 dt 偏小、瞬间 RPM 虚高；阻塞经过的
     * 时间会自然计入下一轮的真实 dt。
     */
    CarControl_UpdateEncoder(nowMs);
    CarControl_ReadEncoderData(&left, &right);
    (void)Track_Update();
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
