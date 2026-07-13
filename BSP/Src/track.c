#include "track.h"

#include <string.h>

#include "bsp_config.h"
#include "adc_track.h"
#include "filter_average.h"

static GPIO_Regs *const g_trackPorts[TRACK_SENSOR_COUNT] = {
    TRACK_D0_PORT, TRACK_D1_PORT, TRACK_D2_PORT, TRACK_D3_PORT,
    TRACK_D4_PORT, TRACK_D5_PORT, TRACK_D6_PORT, TRACK_D7_PORT
};
static const uint32_t g_trackPins[TRACK_SENSOR_COUNT] = {
    TRACK_D0_PIN, TRACK_D1_PIN, TRACK_D2_PIN, TRACK_D3_PIN,
    TRACK_D4_PIN, TRACK_D5_PIN, TRACK_D6_PIN, TRACK_D7_PIN
};
static const int8_t g_trackWeights[TRACK_SENSOR_COUNT] = {-7, -5, -3, -1, 1, 3, 5, 7};
static float g_filterStorage[TRACK_SENSOR_COUNT][TRACK_FILTER_LENGTH];
static AverageFilter_t g_filters[TRACK_SENSOR_COUNT];
static Track_Data_t g_data;
static bool g_externalSamplesReady;

Status_t Track_Init(void)
{
    memset(&g_data, 0, sizeof(g_data));
    for (uint8_t index = 0U; index < TRACK_SENSOR_COUNT; ++index) {
        Status_t status = AverageFilter_Init(&g_filters[index],
            g_filterStorage[index], TRACK_FILTER_LENGTH);
        if (status != STATUS_OK) { return status; }
        g_data.minimum[index] = UINT16_MAX;
        g_data.maximum[index] = 0U;
        g_data.threshold[index] = TRACK_ACTIVE_THRESHOLD;
    }
    g_externalSamplesReady = false;
#if TRACK_ANALOG_MODE
    return ADCTrack_Init();
#else
    return STATUS_OK;
#endif
}

Status_t Track_SetRawSamples(const uint16_t *samples, uint8_t count)
{
    if ((samples == NULL) || (count != TRACK_SENSOR_COUNT)) {
        return STATUS_INVALID_PARAM;
    }
    memcpy(g_data.raw, samples, sizeof(g_data.raw));
    g_externalSamplesReady = true;
    return STATUS_OK;
}

Status_t Track_Update(void)
{
    uint16_t normalized[TRACK_SENSOR_COUNT];
    TrackMath_Result_t result;
    g_data.activeMask = 0U;

    if (!TRACK_ANALOG_MODE) {
        for (uint8_t index = 0U; index < TRACK_SENSOR_COUNT; ++index) {
            bool high = DL_GPIO_readPins(g_trackPorts[index], g_trackPins[index]) != 0U;
            bool active = TRACK_BLACK_IS_HIGH ? high : !high;
            g_data.raw[index] = active ? 1000U : 0U;
        }
    } else if (!g_externalSamplesReady) {
        Status_t adcStatus = ADCTrack_Read(g_data.raw, TRACK_SENSOR_COUNT);
        if (adcStatus != STATUS_OK) { return adcStatus; }
    }
    g_externalSamplesReady = false;

    for (uint8_t index = 0U; index < TRACK_SENSOR_COUNT; ++index) {
        uint16_t value = (uint16_t)AverageFilter_Update(&g_filters[index],
                                                        (float)g_data.raw[index]);
        g_data.filtered[index] = value;
        if (g_data.calibrated && (g_data.maximum[index] > g_data.minimum[index])) {
            uint32_t range = g_data.maximum[index] - g_data.minimum[index];
            int32_t shifted = (int32_t)value - (int32_t)g_data.minimum[index];
            normalized[index] = (uint16_t)Common_ClampInt32(
                (shifted * 1000) / (int32_t)range, 0, 1000);
        } else {
            normalized[index] = value;
        }
        if (!TRACK_BLACK_IS_HIGH) {
            normalized[index] = (uint16_t)(1000U -
                (normalized[index] > 1000U ? 1000U : normalized[index]));
        }
        if (normalized[index] >= g_data.threshold[index]) {
            g_data.activeMask |= (uint8_t)(1U << index);
        }
    }
    (void)TrackMath_Calculate(normalized, g_trackWeights, TRACK_SENSOR_COUNT,
                              TRACK_ACTIVE_THRESHOLD, &result);
    /* 丢线时保留最后一次有效偏差，供上层按原方向低速搜索。 */
    if (result.state != TRACK_STATE_LOST) {
        g_data.positionError = result.position;
    }
    g_data.state = result.state;
    return STATUS_OK;
}

Status_t Track_SetThreshold(uint8_t channel, uint16_t threshold)
{
    if ((channel >= TRACK_SENSOR_COUNT) || (threshold > 1000U)) {
        return STATUS_INVALID_PARAM;
    }
    g_data.threshold[channel] = threshold;
    return STATUS_OK;
}

void Track_CalibrationReset(void)
{
    for (uint8_t index = 0U; index < TRACK_SENSOR_COUNT; ++index) {
        g_data.minimum[index] = UINT16_MAX;
        g_data.maximum[index] = 0U;
    }
    g_data.calibrated = false;
}

void Track_CalibrationFeed(const uint16_t *samples, uint8_t count)
{
    if ((samples == NULL) || (count != TRACK_SENSOR_COUNT)) { return; }
    for (uint8_t index = 0U; index < TRACK_SENSOR_COUNT; ++index) {
        if (samples[index] < g_data.minimum[index]) { g_data.minimum[index] = samples[index]; }
        if (samples[index] > g_data.maximum[index]) { g_data.maximum[index] = samples[index]; }
    }
}

Status_t Track_CalibrationFinish(void)
{
    for (uint8_t index = 0U; index < TRACK_SENSOR_COUNT; ++index) {
        if (g_data.maximum[index] <= g_data.minimum[index]) {
            return STATUS_ERROR;
        }
        g_data.threshold[index] = 500U;
    }
    g_data.calibrated = true;
    return STATUS_OK;
}

const Track_Data_t *Track_GetData(void) { return &g_data; }
float Track_GetPositionError(void) { return g_data.positionError; }
