#include "track.h"

#include <string.h>

#include "i2c.h"
#include "user_config.h"

_Static_assert(TRACK_CHANNEL_COUNT == 8U,
               "I2C eight-channel adapter requires TRACK_CHANNEL_COUNT == 8");

static const float g_trackWeights[TRACK_CHANNEL_COUNT] = {
    -7.0f, -5.0f, -3.0f, -1.0f, 1.0f, 3.0f, 5.0f, 7.0f
};

static Track_Data_t g_data;
static bool g_externalSamplesReady;

static void Track_ApplyModuleMask(uint8_t moduleMask)
{
    for (uint8_t index = 0U; index < TRACK_CHANNEL_COUNT; ++index) {
        uint8_t bit = (uint8_t)(7U - index);
        bool moduleBitHigh = (moduleMask & (uint8_t)(1U << bit)) != 0U;
        bool onBlack = (TRACK_BLACK_IS_HIGH != 0) ?
                       moduleBitHigh : !moduleBitHigh;
        g_data.raw[index] = onBlack ? 1000U : 0U;
    }
}

Status_t Track_Init(void)
{
    (void)memset(&g_data, 0, sizeof(g_data));
    g_externalSamplesReady = false;

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

static Status_t Track_ReadModule(void)
{
    uint8_t moduleMask;
    Status_t status = I2C_ReadRegister(TRACK_I2C_ADDRESS,
                                       TRACK_I2C_STATUS_REGISTER,
                                       &moduleMask, 1U);
    if (status != STATUS_OK) {
        g_data.communicationOk = false;
        g_data.lineFound = false;
        g_data.state = TRACK_STATE_LOST;
        return status;
    }
    Track_ApplyModuleMask(moduleMask);
    g_data.communicationOk = true;
    return STATUS_OK;
}

Status_t Track_Update(void)
{
    uint8_t mask = 0U;
    Status_t status;

    if (g_externalSamplesReady) {
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
            mask |= (uint8_t)(1U << index);
        }
    }

    g_data.activeMask = mask;
    g_data.lineFound = (mask != 0U);
    if (!g_data.lineFound) {
        g_data.state = TRACK_STATE_LOST;
        /* 丢线时保留上一次误差方向，便于控制层低速找线。 */
    } else {
        g_data.positionError = TrackMath_WeightedPosition(
            mask, g_trackWeights, TRACK_CHANNEL_COUNT);
        g_data.state = (mask == 0xFFU) ?
                       TRACK_STATE_ALL_ACTIVE : TRACK_STATE_LINE;
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
