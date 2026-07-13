#include "imu_uart.h"

#include "bsp_config.h"
#include "uart.h"

typedef enum {
    IMU_COMMAND_IDLE = 0,
    IMU_COMMAND_UNLOCK_SENT,
    IMU_COMMAND_ZERO_SENT
} IMU_CommandState_t;

static const uint8_t g_unlockCommand[5] = {0x55U, 0xAAU, 0x13U, 0x8EU, 0x5FU};
static const uint8_t g_zeroCommand[5]   = {0x55U, 0xAAU, 0x15U, 0x00U, 0x00U};
static const uint8_t g_saveCommand[5]   = {0x55U, 0xAAU, 0x00U, 0x00U, 0x00U};
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
    while (UART_ReadByte(UART_ID_IMU, &value) == STATUS_OK) {
        if (ImuProtocol_PushByte(&g_parser, value, nowMs) == STATUS_OK) {
            (void)ImuProtocol_GetSample(&g_parser, &g_sample);
            g_newData = true;
        }
    }

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

Status_t IMU_GetSample(ImuSample_t *sample)
{
    if (sample == NULL) {
        return STATUS_INVALID_PARAM;
    }
    *sample = g_sample;
    g_newData = false;
    return (sample->yawValid || sample->gyroZValid) ? STATUS_OK : STATUS_EMPTY;
}

bool IMU_HasNewData(void)
{
    return g_newData;
}

bool IMU_IsOnline(uint32_t nowMs)
{
    return (g_sample.frameCount > 0U) &&
           ((uint32_t)(nowMs - g_sample.updateTickMs) <= IMU_ONLINE_TIMEOUT_MS);
}

Status_t IMU_StartYawZero(uint32_t nowMs)
{
    Status_t status;
    if (g_commandState != IMU_COMMAND_IDLE) {
        return STATUS_BUSY;
    }
    status = UART_Send(UART_ID_IMU, g_unlockCommand, sizeof(g_unlockCommand));
    if (status == STATUS_OK) {
        g_commandState = IMU_COMMAND_UNLOCK_SENT;
        g_commandTickMs = nowMs;
    }
    return status;
}

