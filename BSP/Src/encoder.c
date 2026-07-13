#include "encoder.h"

#include "bsp_config.h"

#define PI_F 3.14159265358979323846f

typedef struct {
    volatile int32_t count;
    int32_t previousCount;
    Encoder_Data_t data;
} Encoder_State_t;

static Encoder_State_t g_encoder[ENCODER_COUNT];

static bool Encoder_IdIsValid(Encoder_Id_t id)
{
    return (id >= ENCODER_LEFT) && (id < ENCODER_COUNT);
}

Status_t Encoder_Init(void)
{
    for (uint8_t id = 0U; id < (uint8_t)ENCODER_COUNT; ++id) {
        Encoder_Reset((Encoder_Id_t)id);
    }
    NVIC_ClearPendingIRQ(ENCODER_LEFT_IRQn);
    NVIC_EnableIRQ(ENCODER_LEFT_IRQn);
    NVIC_ClearPendingIRQ(ENCODER_RIGHT_IRQn);
    NVIC_EnableIRQ(ENCODER_RIGHT_IRQn);
    return STATUS_OK;
}

void Encoder_OnEdge(Encoder_Id_t id, bool phaseBHigh)
{
    int32_t direction;
    bool reversed;
    if (!Encoder_IdIsValid(id)) {
        return;
    }
    direction = phaseBHigh ? 1 : -1;
    reversed = (id == ENCODER_LEFT) ? ENCODER_LEFT_REVERSED : ENCODER_RIGHT_REVERSED;
    g_encoder[id].count += reversed ? -direction : direction;
}

void Encoder_UpdateSpeed(float sampleTimeS)
{
    float countsPerWheelRev = ENCODER_PULSES_PER_MOTOR_REV *
                              ENCODER_COUNT_MULTIPLIER * ENCODER_GEAR_RATIO;
    if (sampleTimeS <= 0.0f) {
        return;
    }
    for (uint8_t id = 0U; id < (uint8_t)ENCODER_COUNT; ++id) {
        int32_t count = g_encoder[id].count;
        int32_t delta = count - g_encoder[id].previousCount;
        float rpm = ((float)delta * 60.0f) / (countsPerWheelRev * sampleTimeS);
        g_encoder[id].previousCount = count;
        g_encoder[id].data.totalCount = count;
        g_encoder[id].data.deltaCount = delta;
        g_encoder[id].data.rpm += ENCODER_SPEED_FILTER_ALPHA *
                                  (rpm - g_encoder[id].data.rpm);
        g_encoder[id].data.linearSpeedMps =
            g_encoder[id].data.rpm * PI_F * ENCODER_WHEEL_DIAMETER_M / 60.0f;
    }
}

Status_t Encoder_GetData(Encoder_Id_t id, Encoder_Data_t *data)
{
    if (!Encoder_IdIsValid(id) || (data == NULL)) {
        return STATUS_INVALID_PARAM;
    }
    *data = g_encoder[id].data;
    return STATUS_OK;
}

void Encoder_Reset(Encoder_Id_t id)
{
    if (!Encoder_IdIsValid(id)) {
        return;
    }
    g_encoder[id].count = 0;
    g_encoder[id].previousCount = 0;
    g_encoder[id].data = (Encoder_Data_t){0};
}

void Encoder_GPIOIRQHandler(void)
{
    uint32_t statusA = DL_GPIO_getEnabledInterruptStatus(
        ENCODER_LEFT_A_PORT, ENCODER_LEFT_A_PIN);
    uint32_t statusB = DL_GPIO_getEnabledInterruptStatus(
        ENCODER_RIGHT_A_PORT, ENCODER_RIGHT_A_PIN);
    if ((statusA & ENCODER_LEFT_A_PIN) != 0U) {
        Encoder_OnEdge(ENCODER_LEFT,
            DL_GPIO_readPins(ENCODER_LEFT_B_PORT, ENCODER_LEFT_B_PIN) != 0U);
        DL_GPIO_clearInterruptStatus(ENCODER_LEFT_A_PORT, ENCODER_LEFT_A_PIN);
    }
    if ((statusB & ENCODER_RIGHT_A_PIN) != 0U) {
        Encoder_OnEdge(ENCODER_RIGHT,
            DL_GPIO_readPins(ENCODER_RIGHT_B_PORT, ENCODER_RIGHT_B_PIN) != 0U);
        DL_GPIO_clearInterruptStatus(ENCODER_RIGHT_A_PORT, ENCODER_RIGHT_A_PIN);
    }
}

void GROUP1_IRQHandler(void)
{
    Encoder_GPIOIRQHandler();
}
