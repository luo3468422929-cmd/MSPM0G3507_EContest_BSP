#include "motor.h"

#include "bsp_config.h"

typedef struct {
    int16_t targetDuty;
    int16_t appliedDuty;
} Motor_State_t;

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

static void Motor_SetPins(Motor_Id_t id, bool in1, bool in2)
{
    GPIO_Regs *port1 = (id == MOTOR_LEFT) ? MOTOR_AIN1_PORT : MOTOR_BIN1_PORT;
    GPIO_Regs *port2 = (id == MOTOR_LEFT) ? MOTOR_AIN2_PORT : MOTOR_BIN2_PORT;
    uint32_t pin1 = (id == MOTOR_LEFT) ? MOTOR_AIN1_PIN : MOTOR_BIN1_PIN;
    uint32_t pin2 = (id == MOTOR_LEFT) ? MOTOR_AIN2_PIN : MOTOR_BIN2_PIN;
    if (in1) { DL_GPIO_setPins(port1, pin1); } else { DL_GPIO_clearPins(port1, pin1); }
    if (in2) { DL_GPIO_setPins(port2, pin2); } else { DL_GPIO_clearPins(port2, pin2); }
}

static void Motor_Apply(Motor_Id_t id, int16_t duty)
{
    uint32_t compareIndex = (id == MOTOR_LEFT) ?
                            MOTOR_PWM_LEFT_CC_INDEX : MOTOR_PWM_RIGHT_CC_INDEX;
    bool reversed = (id == MOTOR_LEFT) ? MOTOR_LEFT_REVERSED : MOTOR_RIGHT_REVERSED;
    int16_t magnitude;
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
    DL_TimerG_setCaptureCompareValue(MOTOR_PWM_INST, (uint32_t)magnitude, compareIndex);
    g_motor[id].appliedDuty = duty;
}

Status_t Motor_Init(void)
{
    g_enabled = false;
    for (uint8_t id = 0U; id < (uint8_t)MOTOR_COUNT; ++id) {
        g_motor[id].targetDuty = 0;
        g_motor[id].appliedDuty = 0;
        Motor_Apply((Motor_Id_t)id, 0);
    }
    DL_GPIO_clearPins(MOTOR_STBY_PORT, MOTOR_STBY_PIN);
    DL_TimerG_startCounter(MOTOR_PWM_INST);
    return STATUS_OK;
}

Status_t Motor_Enable(bool enable)
{
    g_enabled = enable;
    if (enable) {
        DL_GPIO_setPins(MOTOR_STBY_PORT, MOTOR_STBY_PIN);
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
    DL_TimerG_setCaptureCompareValue(MOTOR_PWM_INST, 0U,
        (id == MOTOR_LEFT) ? MOTOR_PWM_LEFT_CC_INDEX : MOTOR_PWM_RIGHT_CC_INDEX);
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
    DL_GPIO_clearPins(MOTOR_STBY_PORT, MOTOR_STBY_PIN);
    g_enabled = false;
}

void Motor_RampUpdate(void)
{
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
