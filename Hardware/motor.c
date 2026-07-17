/**
 * @file motor.c
 * @brief 实现 TB6612 双路方向控制、PWM 输出、死区补偿、缓升和急停。
 *
 * 所属层：Hardware 执行器层。所有占空比在内部使用“逻辑方向”，最终写
 * 引脚前才应用左右反向配置，避免机械安装方向污染控制算法。
 */
#include "motor.h"

#include "board_pins.h"
#include "user_config.h"

/** 单轮目标与当前输出；缓升器每次只移动 MOTOR_RAMP_STEP。 */
typedef struct {
    int16_t targetDuty;
    int16_t appliedDuty;
} Motor_State_t;

/** 两轮状态和 TB6612 STBY 软件状态，只由主循环控制路径访问。 */
static Motor_State_t g_motor[MOTOR_COUNT];
static bool g_enabled;

static bool Motor_IdIsValid(Motor_Id_t id)
{
    return (id >= MOTOR_LEFT) && (id < MOTOR_COUNT);
}

static int16_t Motor_LimitDuty(int16_t duty)
{
    return (int16_t)Common_ClampInt32(duty, -MOTOR_MAX_DUTY, MOTOR_MAX_DUTY);
}

/** 将方向脚写成 IN1/IN2 指定电平；不改变 PWM。 */
static void Motor_SetPins(Motor_Id_t id, bool in1, bool in2)
{
    GPIO_Regs *port1 = (id == MOTOR_LEFT) ? PIN_MOTOR_AIN1_PORT : PIN_MOTOR_BIN1_PORT;
    GPIO_Regs *port2 = (id == MOTOR_LEFT) ? PIN_MOTOR_AIN2_PORT : PIN_MOTOR_BIN2_PORT;
    uint32_t pin1 = (id == MOTOR_LEFT) ? PIN_MOTOR_AIN1 : PIN_MOTOR_BIN1;
    uint32_t pin2 = (id == MOTOR_LEFT) ? PIN_MOTOR_AIN2 : PIN_MOTOR_BIN2;
    if (in1) { DL_GPIO_setPins(port1, pin1); } else { DL_GPIO_clearPins(port1, pin1); }
    if (in2) { DL_GPIO_setPins(port2, pin2); } else { DL_GPIO_clearPins(port2, pin2); }
}

static void Motor_Apply(Motor_Id_t id, int16_t duty)
{
    uint32_t compareIndex = (id == MOTOR_LEFT) ?
                            PIN_MOTOR_LEFT_CC_INDEX : PIN_MOTOR_RIGHT_CC_INDEX;
    bool reversed = (id == MOTOR_LEFT) ? MOTOR_LEFT_REVERSED : MOTOR_RIGHT_REVERSED;
    int16_t logicalDuty = duty;
    int16_t magnitude;
    /* 反向宏只改变最终物理输出，PID/缓升仍看到统一的逻辑正方向。 */
    if (reversed) {
        duty = (int16_t)-duty;
    }
    if (duty > 0) {
        Motor_SetPins(id, true, false);
        magnitude = duty;
    } else if (duty < 0) {
        Motor_SetPins(id, false, true);
        magnitude = (int16_t)-duty;
    } else {
        Motor_SetPins(id, false, false);
        magnitude = 0;
    }
    DL_TimerG_setCaptureCompareValue(PIN_MOTOR_PWM_INST, (uint32_t)magnitude, compareIndex);
    /* 斜坡状态保存逻辑占空比，不能保存方向翻转后的物理占空比。 */
    g_motor[id].appliedDuty = logicalDuty;
}

Status_t Motor_Init(void)
{
    /* 先把方向和 PWM 清零，再拉低 STBY，最后才启动已配置的 PWM 计数器。 */
    g_enabled = false;
    for (uint8_t id = 0U; id < (uint8_t)MOTOR_COUNT; ++id) {
        g_motor[id].targetDuty = 0;
        g_motor[id].appliedDuty = 0;
        Motor_Apply((Motor_Id_t)id, 0);
    }
    DL_GPIO_clearPins(PIN_MOTOR_STBY_PORT, PIN_MOTOR_STBY);
    DL_TimerG_startCounter(PIN_MOTOR_PWM_INST);
    return STATUS_OK;
}

Status_t Motor_Enable(bool enable)
{
    g_enabled = enable;
    if (enable) {
        DL_GPIO_setPins(PIN_MOTOR_STBY_PORT, PIN_MOTOR_STBY);
    } else {
        Motor_EmergencyStop();
    }
    return STATUS_OK;
}

Status_t Motor_SetDuty(Motor_Id_t id, int16_t duty)
{
    if (!Motor_IdIsValid(id)) {
        return STATUS_INVALID_PARAM;
    }
    if (!g_enabled) {
        return STATUS_DISABLED;
    }
    duty = Motor_LimitDuty(duty);
    /* 补偿减速电机静摩擦；若低速抖动，应首先重新测量这个阈值。 */
    if ((duty != 0) && (duty < MOTOR_MIN_START_DUTY) && (duty > -MOTOR_MIN_START_DUTY)) {
        duty = (duty > 0) ? MOTOR_MIN_START_DUTY : -MOTOR_MIN_START_DUTY;
    }
    g_motor[id].targetDuty = duty;
    return STATUS_OK;
}

Status_t Motor_SetDutyPair(int16_t leftDuty, int16_t rightDuty)
{
    Status_t leftStatus = Motor_SetDuty(MOTOR_LEFT, leftDuty);
    Status_t rightStatus = Motor_SetDuty(MOTOR_RIGHT, rightDuty);
    return (leftStatus != STATUS_OK) ? leftStatus : rightStatus;
}

Status_t Motor_Stop(Motor_Id_t id, Motor_StopMode_t mode)
{
    if (!Motor_IdIsValid(id)) {
        return STATUS_INVALID_PARAM;
    }
    g_motor[id].targetDuty = 0;
    g_motor[id].appliedDuty = 0;
    DL_TimerG_setCaptureCompareValue(PIN_MOTOR_PWM_INST, 0U,
        (id == MOTOR_LEFT) ? PIN_MOTOR_LEFT_CC_INDEX : PIN_MOTOR_RIGHT_CC_INDEX);
    /* BRAKE 时两输入同为高；COAST 时两输入同为低。 */
    Motor_SetPins(id, mode == MOTOR_STOP_BRAKE, mode == MOTOR_STOP_BRAKE);
    return STATUS_OK;
}

void Motor_StopAll(void)
{
    (void)Motor_Stop(MOTOR_LEFT, MOTOR_STOP_COAST);
    (void)Motor_Stop(MOTOR_RIGHT, MOTOR_STOP_COAST);
}

void Motor_EmergencyStop(void)
{
    Motor_StopAll();
    DL_GPIO_clearPins(PIN_MOTOR_STBY_PORT, PIN_MOTOR_STBY);
    g_enabled = false;
}

void Motor_RampUpdate(void)
{
    /* 调用周期会直接影响实际斜坡时间，当前由 10 ms 控制任务调用。 */
    for (uint8_t id = 0U; id < (uint8_t)MOTOR_COUNT; ++id) {
        int16_t applied = g_motor[id].appliedDuty;
        int16_t target = g_motor[id].targetDuty;
        if (applied < target) {
            applied = (int16_t)(applied + MOTOR_RAMP_STEP);
            if (applied > target) { applied = target; }
        } else if (applied > target) {
            applied = (int16_t)(applied - MOTOR_RAMP_STEP);
            if (applied < target) { applied = target; }
        }
        Motor_Apply((Motor_Id_t)id, applied);
    }
}

int16_t Motor_GetAppliedDuty(Motor_Id_t id)
{
    return Motor_IdIsValid(id) ? g_motor[id].appliedDuty : 0;
}
