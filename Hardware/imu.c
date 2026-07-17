/**
 * @file imu.c
 * @brief 实现 NCU 惯导 UART 数据消费、在线状态和非阻塞航向清零命令序列。
 *
 * 所属层：Hardware 传感器层。UART2 ISR 已把字节放入环形缓冲，本文件在
 * 20 ms 主循环任务中解析；LCD 使用 Peek，算法使用 Get，避免互相抢标志。
 */
#include "imu.h"

#include "user_config.h"
#include "uart.h"

/** 航向清零命令的非阻塞阶段。 */
typedef enum {
    IMU_COMMAND_IDLE = 0,      /**< 没有命令序列。 */
    IMU_COMMAND_UNLOCK_SENT,   /**< 已解锁，等待 100 ms 后发送清零。 */
    IMU_COMMAND_ZERO_SENT      /**< 已清零，等待 100 ms 后保存。 */
} IMU_CommandState_t;

/* 商家协议固定命令；每条均为 5 字节，发送顺序不能交换。 */
static const uint8_t g_unlockCommand[5] = {0x55U, 0xAAU, 0x13U, 0x8EU, 0x5FU};
static const uint8_t g_zeroCommand[5]   = {0x55U, 0xAAU, 0x15U, 0x00U, 0x00U};
static const uint8_t g_saveCommand[5]   = {0x55U, 0xAAU, 0x00U, 0x00U, 0x00U};
/* 解析器、对外样本和命令状态均由主循环访问，不在 UART ISR 中修改。 */
static ImuProtocol_t g_parser;
static ImuSample_t g_sample;
static bool g_newData;
static IMU_CommandState_t g_commandState;
static uint32_t g_commandTickMs;

Status_t IMU_Init(void)
{
    ImuProtocol_Init(&g_parser);
    g_sample = (ImuSample_t){0};
    g_newData = false;
    g_commandState = IMU_COMMAND_IDLE;
    return STATUS_OK;
}

void IMU_Process(uint32_t nowMs)
{
    uint8_t value;
    /* 一次调用清空当前 UART 缓冲，完整帧会更新 g_sample 和新数据标志。 */
    while (UART_ReadByte(UART_ID_IMU, &value) == STATUS_OK) {
        if (ImuProtocol_PushByte(&g_parser, value, nowMs) == STATUS_OK) {
            (void)ImuProtocol_GetSample(&g_parser, &g_sample);
            g_newData = true;
        }
    }

    /* 分阶段等待代替 DelayMs，清零期间主循环和电机控制仍可运行。 */
    if ((g_commandState == IMU_COMMAND_UNLOCK_SENT) &&
        ((uint32_t)(nowMs - g_commandTickMs) >= 100U)) {
        if (UART_Send(UART_ID_IMU, g_zeroCommand, sizeof(g_zeroCommand)) == STATUS_OK) {
            g_commandState = IMU_COMMAND_ZERO_SENT;
            g_commandTickMs = nowMs;
        }
    } else if ((g_commandState == IMU_COMMAND_ZERO_SENT) &&
               ((uint32_t)(nowMs - g_commandTickMs) >= 100U)) {
        if (UART_Send(UART_ID_IMU, g_saveCommand, sizeof(g_saveCommand)) == STATUS_OK) {
            g_commandState = IMU_COMMAND_IDLE;
        }
    }
}

Status_t IMU_PeekSample(ImuSample_t *sample)
{
    if (sample == NULL) {
        return STATUS_INVALID_PARAM;
    }
    *sample = g_sample;
    return (sample->yawValid || sample->gyroZValid) ? STATUS_OK : STATUS_EMPTY;
}

Status_t IMU_GetSample(ImuSample_t *sample)
{
    Status_t status = IMU_PeekSample(sample);
    /* 即使当前无有效字段，合法指针的一次 Get 也视为消费查询。 */
    if (status != STATUS_INVALID_PARAM) {
        g_newData = false;
    }
    return status;
}

bool IMU_HasNewData(void)
{
    return g_newData;
}

bool IMU_IsOnline(uint32_t nowMs)
{
    /* 无符号时间差兼容系统 tick 回绕。 */
    return (g_sample.frameCount > 0U) &&
           ((uint32_t)(nowMs - g_sample.updateTickMs) <= IMU_ONLINE_TIMEOUT_MS);
}

Status_t IMU_StartYawZero(uint32_t nowMs)
{
    Status_t status;
    if (g_commandState != IMU_COMMAND_IDLE) {
        return STATUS_BUSY;
    }
    /* 只在解锁发送成功后进入状态机，发送失败可由上层稍后重试。 */
    status = UART_Send(UART_ID_IMU, g_unlockCommand, sizeof(g_unlockCommand));
    if (status == STATUS_OK) {
        g_commandState = IMU_COMMAND_UNLOCK_SENT;
        g_commandTickMs = nowMs;
    }
    return status;
}

