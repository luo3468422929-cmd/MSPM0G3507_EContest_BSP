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

void Encoder_OnEdge(Encoder_Id_t id, Encoder_Edge_t edge,
                    bool phaseAHigh, bool phaseBHigh)
{
    int8_t direction;
    bool reversed;
    if (!Encoder_IdIsValid(id)) {
        return;
    }
    direction = EncoderDecode_GetIncrement(edge, phaseAHigh, phaseBHigh);
    if (direction == 0) {
        return;
    }
    reversed = (id == ENCODER_LEFT) ? ENCODER_LEFT_REVERSED : ENCODER_RIGHT_REVERSED;
    g_encoder[id].count += reversed ? -direction : direction;
}

void Encoder_UpdateSpeed(float sampleTimeS)
{
    float countsPerWheelRev = ENCODER_COUNTS_PER_WHEEL_REV;
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

static void Encoder_HandleLeftARising(void)
{
    bool phaseAHigh = DL_GPIO_readPins(PIN_ENCODER_LEFT_A_PORT,
                                       PIN_ENCODER_LEFT_A) != 0U;
    bool phaseBHigh = DL_GPIO_readPins(PIN_ENCODER_LEFT_B_PORT,
                                       PIN_ENCODER_LEFT_B) != 0U;
    Encoder_OnEdge(ENCODER_LEFT, ENCODER_EDGE_A_RISING,
                   phaseAHigh, phaseBHigh);
}

static void Encoder_HandleLeftBRising(void)
{
    bool phaseAHigh = DL_GPIO_readPins(PIN_ENCODER_LEFT_A_PORT,
                                       PIN_ENCODER_LEFT_A) != 0U;
    bool phaseBHigh = DL_GPIO_readPins(PIN_ENCODER_LEFT_B_PORT,
                                       PIN_ENCODER_LEFT_B) != 0U;
    Encoder_OnEdge(ENCODER_LEFT, ENCODER_EDGE_B_RISING,
                   phaseAHigh, phaseBHigh);
}

static void Encoder_HandleRightARising(void)
{
    bool phaseAHigh = DL_GPIO_readPins(PIN_ENCODER_RIGHT_A_PORT,
                                       PIN_ENCODER_RIGHT_A) != 0U;
    bool phaseBHigh = DL_GPIO_readPins(PIN_ENCODER_RIGHT_B_PORT,
                                       PIN_ENCODER_RIGHT_B) != 0U;
    Encoder_OnEdge(ENCODER_RIGHT, ENCODER_EDGE_A_RISING,
                   phaseAHigh, phaseBHigh);
}

static void Encoder_HandleRightBRising(void)
{
    bool phaseAHigh = DL_GPIO_readPins(PIN_ENCODER_RIGHT_A_PORT,
                                       PIN_ENCODER_RIGHT_A) != 0U;
    bool phaseBHigh = DL_GPIO_readPins(PIN_ENCODER_RIGHT_B_PORT,
                                       PIN_ENCODER_RIGHT_B) != 0U;
    Encoder_OnEdge(ENCODER_RIGHT, ENCODER_EDGE_B_RISING,
                   phaseAHigh, phaseBHigh);
}

void GROUP1_IRQHandler(void)
{
    uint32_t gpioAStatus = DL_GPIO_getEnabledInterruptStatus(
        PIN_ENCODER_LEFT_A_PORT, PIN_ENCODER_LEFT_A | PIN_ENCODER_LEFT_B);
    uint32_t gpioBStatus = DL_GPIO_getEnabledInterruptStatus(
        PIN_ENCODER_RIGHT_A_PORT, PIN_ENCODER_RIGHT_A | PIN_ENCODER_RIGHT_B);

    if ((gpioAStatus & PIN_ENCODER_LEFT_A) != 0U) {
        Encoder_HandleLeftARising();
        DL_GPIO_clearInterruptStatus(PIN_ENCODER_LEFT_A_PORT,
                                     PIN_ENCODER_LEFT_A);
    }
    if ((gpioAStatus & PIN_ENCODER_LEFT_B) != 0U) {
        Encoder_HandleLeftBRising();
        DL_GPIO_clearInterruptStatus(PIN_ENCODER_LEFT_B_PORT,
                                     PIN_ENCODER_LEFT_B);
    }
    if ((gpioBStatus & PIN_ENCODER_RIGHT_A) != 0U) {
        Encoder_HandleRightARising();
        DL_GPIO_clearInterruptStatus(PIN_ENCODER_RIGHT_A_PORT,
                                     PIN_ENCODER_RIGHT_A);
    }
    if ((gpioBStatus & PIN_ENCODER_RIGHT_B) != 0U) {
        Encoder_HandleRightBRising();
        DL_GPIO_clearInterruptStatus(PIN_ENCODER_RIGHT_B_PORT,
                                     PIN_ENCODER_RIGHT_B);
    }
}
