/**
 * @file track.c
 * @brief 实现 Path Fish 12 路 I2C 循迹模块的半帧拼接、状态和位置误差。
 *
 * 所属层：Hardware 传感器层。模块一次 I2C 请求只返回 X1~X6 或 X7~X12
 * 的 7 字节半帧，本文件把两半拼成统一位图。持续通信异常时立即把
 * communicationOk/lineFound 清零，使控制层停车，不用旧帧继续驱动。
 */
#include "track.h"

#include <string.h>

#include "i2c.h"
#include "track_protocol.h"
#include "user_config.h"

_Static_assert(TRACK_CHANNEL_COUNT == 12U,
               "Path Fish adapter requires TRACK_CHANNEL_COUNT == 12");

/* X1~X12 从左到右映射到 -7~+7，保持原循迹位置环的误差量级。 */
static const float g_trackWeights[TRACK_CHANNEL_COUNT] = {
    -7.0000f, -5.7273f, -4.4545f, -3.1818f, -1.9091f, -0.6364f,
     0.6364f,  1.9091f,  3.1818f,  4.4545f,  5.7273f,  7.0000f
};

static Track_Data_t g_data;
static TrackProtocol_t g_protocol;
static bool g_externalSamplesReady;
static bool g_hasCompleteFrame;
static uint8_t g_incompleteUpdates;

/** 把协议位图转换成项目统一的“bit0=X1 且 1=黑线”样本。 */
static void Track_ApplyModuleMask(uint16_t moduleMask)
{
    for (uint8_t physicalIndex = 0U;
         physicalIndex < TRACK_CHANNEL_COUNT; ++physicalIndex) {
        uint8_t logicalIndex = (TRACK_SENSOR_REVERSED != 0) ?
            (uint8_t)((TRACK_CHANNEL_COUNT - 1U) - physicalIndex) :
            physicalIndex;
        bool moduleBitHigh =
            (moduleMask & (uint16_t)(1U << physicalIndex)) != 0U;
        bool onBlack = (TRACK_BLACK_IS_HIGH != 0) ?
            moduleBitHigh : !moduleBitHigh;

        g_data.raw[logicalIndex] = onBlack ? 1000U : 0U;
    }
}

/** 统一设置通信故障状态，防止控制层误用失效数据。 */
static void Track_SetCommunicationFault(void)
{
    g_hasCompleteFrame = false;
    g_data.communicationOk = false;
    g_data.lineFound = false;
    g_data.state = TRACK_STATE_LOST;
}

Status_t Track_Init(void)
{
    (void)memset(&g_data, 0, sizeof(g_data));
    TrackProtocol_Reset(&g_protocol);
    g_externalSamplesReady = false;
    g_hasCompleteFrame = false;
    g_incompleteUpdates = 0U;

    for (uint8_t index = 0U; index < TRACK_CHANNEL_COUNT; ++index) {
        g_data.minimum[index] = UINT16_MAX;
        g_data.threshold[index] = TRACK_ACTIVE_THRESHOLD;
    }
    g_data.state = TRACK_STATE_LOST;
    g_data.communicationOk = false;
    return STATUS_OK;
}

Status_t Track_SetRawSamples(const uint16_t *samples, uint8_t count)
{
    if ((samples == NULL) || (count != TRACK_CHANNEL_COUNT)) {
        return STATUS_INVALID_PARAM;
    }
    (void)memcpy(g_data.raw, samples, sizeof(g_data.raw));
    g_externalSamplesReady = true;
    return STATUS_OK;
}

/**
 * 从模块读取半帧。正常情况下连续两次请求分别得到 # 和 !；若模块暂时
 * 重复同一半帧，则跨 Track_Update 保留已收到的一半，并短暂沿用上一完整帧。
 */
static Status_t Track_ReadModule(void)
{
    uint8_t halfFrame[TRACK_PROTOCOL_HALF_FRAME_SIZE];
    uint16_t completeMask = 0U;

    for (uint8_t readIndex = 0U;
         readIndex < TRACK_I2C_HALF_READS_PER_UPDATE; ++readIndex) {
        Status_t status = I2C_Read(TRACK_I2C_ADDRESS, halfFrame,
                                   TRACK_PROTOCOL_HALF_FRAME_SIZE);
        if (status != STATUS_OK) {
            TrackProtocol_Reset(&g_protocol);
            g_incompleteUpdates = 0U;
            Track_SetCommunicationFault();
            return status;
        }

        status = TrackProtocol_PushHalf(&g_protocol, halfFrame,
                                        sizeof(halfFrame), &completeMask);
        if (status == STATUS_OK) {
            Track_ApplyModuleMask(completeMask);
            g_hasCompleteFrame = true;
            g_incompleteUpdates = 0U;
            g_data.communicationOk = true;
            return STATUS_OK;
        }
        if (status != STATUS_BUSY) {
            g_incompleteUpdates = 0U;
            Track_SetCommunicationFault();
            return status;
        }
    }

    if (g_incompleteUpdates < UINT8_MAX) {
        ++g_incompleteUpdates;
    }
    if (g_incompleteUpdates <= TRACK_I2C_STALE_UPDATE_LIMIT) {
        if (g_hasCompleteFrame) {
            /* 仅缺当前另一半时短暂沿用上一完整帧，避免正常交替边界误停车。 */
            g_data.communicationOk = true;
            return STATUS_OK;
        }
        /* 首次上电尚缺一半：保留协议缓存，但绝不把未完整数据交给控制层。 */
        g_data.communicationOk = false;
        g_data.lineFound = false;
        g_data.state = TRACK_STATE_LOST;
        return STATUS_BUSY;
    }

    bool hadCompleteFrame = g_hasCompleteFrame;
    TrackProtocol_Reset(&g_protocol);
    Track_SetCommunicationFault();
    return hadCompleteFrame ? STATUS_TIMEOUT : STATUS_BUSY;
}

Status_t Track_Update(void)
{
    uint16_t mask = 0U;
    Status_t status;

    if (g_externalSamplesReady) {
        /* Host/离线注入只覆盖下一次更新，之后自动恢复真实 I2C 读取。 */
        g_externalSamplesReady = false;
        g_data.communicationOk = true;
    } else {
        status = Track_ReadModule();
        if (status != STATUS_OK) {
            return status;
        }
    }

    for (uint8_t index = 0U; index < TRACK_CHANNEL_COUNT; ++index) {
        g_data.filtered[index] = g_data.raw[index];
        if (g_data.filtered[index] >= g_data.threshold[index]) {
            mask |= (uint16_t)(1U << index);
        }
    }

    g_data.activeMask = mask;
    if (mask == 0U) {
        g_data.lineFound = false;
        g_data.state = TRACK_STATE_LOST;
        /* 丢线时保留上一次误差方向，控制层可据此实现低速找线。 */
    } else {
        g_data.positionError = TrackMath_WeightedPosition(
            mask, g_trackWeights, TRACK_CHANNEL_COUNT);
        if (mask == TRACK_PROTOCOL_FULL_MASK) {
            g_data.state = TRACK_STATE_ALL_ACTIVE;
            g_data.lineFound = (TRACK_ALL_ACTIVE_IS_LINE != 0);
        } else {
            g_data.state = TRACK_STATE_LINE;
            g_data.lineFound = true;
        }
    }
    return STATUS_OK;
}

Status_t Track_SetThreshold(uint8_t channel, uint16_t threshold)
{
    if ((channel >= TRACK_CHANNEL_COUNT) || (threshold > 1000U)) {
        return STATUS_INVALID_PARAM;
    }
    g_data.threshold[channel] = threshold;
    return STATUS_OK;
}

void Track_CalibrationReset(void)
{
    for (uint8_t index = 0U; index < TRACK_CHANNEL_COUNT; ++index) {
        g_data.minimum[index] = UINT16_MAX;
        g_data.maximum[index] = 0U;
    }
    g_data.calibrated = false;
}

void Track_CalibrationFeed(const uint16_t *samples, uint8_t count)
{
    if ((samples == NULL) || (count != TRACK_CHANNEL_COUNT)) {
        return;
    }
    for (uint8_t index = 0U; index < TRACK_CHANNEL_COUNT; ++index) {
        if (samples[index] < g_data.minimum[index]) {
            g_data.minimum[index] = samples[index];
        }
        if (samples[index] > g_data.maximum[index]) {
            g_data.maximum[index] = samples[index];
        }
    }
}

Status_t Track_CalibrationFinish(void)
{
    for (uint8_t index = 0U; index < TRACK_CHANNEL_COUNT; ++index) {
        if ((g_data.minimum[index] == UINT16_MAX) ||
            (g_data.maximum[index] <= g_data.minimum[index])) {
            return STATUS_ERROR;
        }
        g_data.threshold[index] = (uint16_t)(g_data.minimum[index] +
            ((g_data.maximum[index] - g_data.minimum[index]) / 2U));
    }
    g_data.calibrated = true;
    return STATUS_OK;
}

const Track_Data_t *Track_GetData(void)
{
    return &g_data;
}

float Track_GetPositionError(void)
{
    return g_data.positionError;
}
