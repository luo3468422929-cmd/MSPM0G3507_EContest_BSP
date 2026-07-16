#include "encoder.h"

#include "board_pins.h"
#include "encoder_speed_window.h"
#include "user_config.h"

#define PI_F 3.14159265358979323846f

typedef struct {
    volatile int32_t count;
    int32_t previousCount;
    EncoderSpeedWindow_t speedWindow;
    Encoder_Data_t data;
    bool speedInitialized;
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
    NVIC_ClearPendingIRQ(PIN_ENCODER_GPIOA_IRQn);
    NVIC_EnableIRQ(PIN_ENCODER_GPIOA_IRQn);
    NVIC_ClearPendingIRQ(PIN_ENCODER_GPIOB_IRQn);
    NVIC_EnableIRQ(PIN_ENCODER_GPIOB_IRQn);
    return STATUS_OK;
}

void Encoder_OnEdge(Encoder_Id_t id, bool phaseAHigh, bool phaseBHigh)
{
    int32_t direction;
    bool reversed;
    if (!Encoder_IdIsValid(id)) {
        return;
    }
    /* A 相双边沿计数：A/B 同相和异相分别代表两个运动方向。 */
    direction = (phaseAHigh == phaseBHigh) ? 1 : -1;
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
        float rpm = 0.0f;
        bool ready = false;

        g_encoder[id].previousCount = count;
        g_encoder[id].data.totalCount = count;
        g_encoder[id].data.deltaCount = delta;
        if ((EncoderSpeedWindow_Push(&g_encoder[id].speedWindow,
                                     delta, sampleTimeS,
                                     ENCODER_SPEED_WINDOW_S,
                                     countsPerWheelRev, &rpm, &ready) !=
             STATUS_OK) || !ready) {
            /* 时间窗未满时保留上一次有效速度，避免启动瞬间误放大。 */
            continue;
        }
        if (!g_encoder[id].speedInitialized) {
            g_encoder[id].data.rpm = rpm;
            g_encoder[id].speedInitialized = true;
        } else {
            g_encoder[id].data.rpm += ENCODER_SPEED_FILTER_ALPHA *
                                      (rpm - g_encoder[id].data.rpm);
        }
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
    (void)EncoderSpeedWindow_Init(&g_encoder[id].speedWindow);
    g_encoder[id].data = (Encoder_Data_t){0};
    g_encoder[id].speedInitialized = false;
}

static void Encoder_HandleLeftEdge(void)
{
    bool phaseAHigh = DL_GPIO_readPins(PIN_ENCODER_LEFT_A_PORT,
                                       PIN_ENCODER_LEFT_A) != 0U;
    bool phaseBHigh = DL_GPIO_readPins(PIN_ENCODER_LEFT_B_PORT,
                                       PIN_ENCODER_LEFT_B) != 0U;
    Encoder_OnEdge(ENCODER_LEFT, phaseAHigh, phaseBHigh);
}

static void Encoder_HandleRightEdge(void)
{
    bool phaseAHigh = DL_GPIO_readPins(PIN_ENCODER_RIGHT_A_PORT,
                                       PIN_ENCODER_RIGHT_A) != 0U;
    bool phaseBHigh = DL_GPIO_readPins(PIN_ENCODER_RIGHT_B_PORT,
                                       PIN_ENCODER_RIGHT_B) != 0U;
    Encoder_OnEdge(ENCODER_RIGHT, phaseAHigh, phaseBHigh);
}

void GROUP1_IRQHandler(void)
{
    uint32_t gpioAStatus = DL_GPIO_getEnabledInterruptStatus(
        PIN_ENCODER_LEFT_A_PORT, PIN_ENCODER_LEFT_A);
    uint32_t gpioBStatus = DL_GPIO_getEnabledInterruptStatus(
        PIN_ENCODER_RIGHT_A_PORT, PIN_ENCODER_RIGHT_A);

    if (gpioAStatus != 0U) {
        Encoder_HandleLeftEdge();
        DL_GPIO_clearInterruptStatus(PIN_ENCODER_LEFT_A_PORT,
                                     PIN_ENCODER_LEFT_A);
    }
    if (gpioBStatus != 0U) {
        Encoder_HandleRightEdge();
        DL_GPIO_clearInterruptStatus(PIN_ENCODER_RIGHT_A_PORT,
                                     PIN_ENCODER_RIGHT_A);
    }
}
