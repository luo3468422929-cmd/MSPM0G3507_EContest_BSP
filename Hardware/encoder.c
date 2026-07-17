/**
 * @file encoder.c
 * @brief 实现四路 GPIO 上升沿 x2 正交解码和基于真实 dt 的双轮测速。
 *
 * 所属层：Hardware 传感器层。ISR 只更新 volatile count；主循环定期复制
 * 计数并完成滑动时间窗、低通滤波及线速度换算。
 */
#include "encoder.h"

#include "board_pins.h"
#include "encoder_speed_window.h"
#include "user_config.h"

/** 轮周长换算使用的单精度圆周率。 */
#define PI_F 3.14159265358979323846f

/** 单轮 ISR 计数、上次快照、时间窗和对外数据。 */
typedef struct {
    volatile int32_t count; /**< GPIO ISR 写入的有符号累计计数。 */
    int32_t previousCount;  /**< 上次速度更新时的 count 快照。 */
    EncoderSpeedWindow_t speedWindow; /**< 低速稳定用固定时间窗。 */
    Encoder_Data_t data;    /**< 提供给控制层的数据快照。 */
    bool speedInitialized;  /**< 首次有效 RPM 是否已直接装载。 */
} Encoder_State_t;

static Encoder_State_t g_encoder[ENCODER_COUNT];

static bool Encoder_IdIsValid(Encoder_Id_t id)
{
    return (id >= ENCODER_LEFT) && (id < ENCODER_COUNT);
}

Status_t Encoder_Init(void)
{
    /* 先清状态，再清 NVIC 待决标志并开放中断，避免把上电毛刺计入速度。 */
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
    /* 方向修正只由配置宏完成，不修改公共解码公式。 */
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

        /* 先更新快照；即使时间窗未满，累计/增量计数仍对调试可见。 */
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
            /* 首个有效值直接赋值，避免滤波器从 0 缓慢爬升。 */
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
    /* 每个边沿到来时同时读取 A/B 当前电平，交给纯 x2 判向函数。 */
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
    /* GROUP1 同时承载 GPIOA/GPIOB；先保存状态，防止处理中途新边沿被混淆。 */
    uint32_t gpioAStatus = DL_GPIO_getEnabledInterruptStatus(
        PIN_ENCODER_LEFT_A_PORT, PIN_ENCODER_LEFT_A | PIN_ENCODER_LEFT_B);
    uint32_t gpioBStatus = DL_GPIO_getEnabledInterruptStatus(
        PIN_ENCODER_RIGHT_A_PORT, PIN_ENCODER_RIGHT_A | PIN_ENCODER_RIGHT_B);

    /* 四路必须使用独立 if；同一中断内可能同时有多个有效边沿。 */
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
