#include "track.h"

#include <string.h>

#include "board_pins.h"
#include "filter_average.h"

_Static_assert(TRACK_CHANNEL_COUNT == 5U,
               "GPIO five-channel adapter requires TRACK_CHANNEL_COUNT == 5");

static GPIO_Regs *const g_trackPorts[TRACK_CHANNEL_COUNT] = {
    PIN_TRACK_CH1_PORT, PIN_TRACK_CH2_PORT, PIN_TRACK_CH3_PORT,
    PIN_TRACK_CH4_PORT, PIN_TRACK_CH5_PORT
};

static const uint32_t g_trackPins[TRACK_CHANNEL_COUNT] = {
    PIN_TRACK_CH1, PIN_TRACK_CH2, PIN_TRACK_CH3, PIN_TRACK_CH4, PIN_TRACK_CH5
};

static const float g_trackWeights[TRACK_CHANNEL_COUNT] = {
    -4.0f, -2.0f, 0.0f, 2.0f, 4.0f
};

static float g_filterStorage[TRACK_CHANNEL_COUNT][TRACK_FILTER_LENGTH];
static AverageFilter_t g_filters[TRACK_CHANNEL_COUNT];
static Track_Data_t g_data;
static bool g_externalSamplesReady;

Status_t Track_Init(void)
{
    memset(&g_data, 0, sizeof(g_data));
    memset(g_filterStorage, 0, sizeof(g_filterStorage));
    g_externalSamplesReady = false;

    for (uint8_t index = 0U; index < TRACK_CHANNEL_COUNT; ++index) {
        Status_t status = AverageFilter_Init(&g_filters[index],
            g_filterStorage[index], TRACK_FILTER_LENGTH);
        if (status != STATUS_OK) {
            return status;
        }
        g_data.minimum[index] = UINT16_MAX;
        g_data.threshold[index] = TRACK_ACTIVE_THRESHOLD;
    }
    g_data.state = TRACK_STATE_LOST;
    return STATUS_OK;
}

Status_t Track_SetRawSamples(const uint16_t *samples, uint8_t count)
{
    if ((samples == NULL) || (count != TRACK_CHANNEL_COUNT)) {
        return STATUS_INVALID_PARAM;
    }
    memcpy(g_data.raw, samples, sizeof(g_data.raw));
    g_externalSamplesReady = true;
    return STATUS_OK;
}

static void Track_ReadDigitalInputs(void)
{
    for (uint8_t index = 0U; index < TRACK_CHANNEL_COUNT; ++index) {
        bool pinHigh = DL_GPIO_readPins(g_trackPorts[index], g_trackPins[index]) != 0U;
        bool onBlack = (TRACK_BLACK_IS_HIGH != 0) ? pinHigh : !pinHigh;
        g_data.raw[index] = onBlack ? 1000U : 0U;
    }
}

Status_t Track_Update(void)
{
    uint8_t mask = 0U;

    if (!g_externalSamplesReady) {
        Track_ReadDigitalInputs();
    }
    g_externalSamplesReady = false;

    for (uint8_t index = 0U; index < TRACK_CHANNEL_COUNT; ++index) {
        float filtered = AverageFilter_Update(&g_filters[index],
                                               (float)g_data.raw[index]);
        if (filtered < 0.0f) {
            filtered = 0.0f;
        } else if (filtered > 1000.0f) {
            filtered = 1000.0f;
        }
        g_data.filtered[index] = (uint16_t)(filtered + 0.5f);
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
        g_data.state = (mask == (uint8_t)((1U << TRACK_CHANNEL_COUNT) - 1U)) ?
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
